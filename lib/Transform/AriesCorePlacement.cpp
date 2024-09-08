#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Analysis/Liveness.h"
#include "mlir/Dialect/Affine/Utils.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/Dialect/Affine/Analysis/LoopAnalysis.h"
#include "llvm/Support/Debug.h"
#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "aries/Dialect/ADF/ADFDialect.h"

using namespace mlir;
using namespace aries;
using namespace adf;
using namespace mlir::memref;
using namespace mlir::func;
using namespace mlir::affine;

namespace {

struct AriesCorePlacement : public AriesCorePlacementBase<AriesCorePlacement> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";
  
    if (!CorePlacement(mod,topFuncName))
      signalPassFailure();
  }

private:
  bool placementNaive1(OpBuilder builder, CallOp callOp, int64_t height,
                      int64_t colNum, int64_t rowNum, int64_t colStart, 
                      int64_t rowStart, int64_t KSize, int64_t JSize, 
                      int64_t ISize, bool flow_flag, 
                      SmallVector<int64_t, 3> ivIdeices){
    auto indexType = builder.getIndexType();
    int64_t kSize = 1;
    int64_t jSize = 1;
    int64_t iSize = 1;
    if(flow_flag){
      kSize = ivIdeices[0];
      if(ivIdeices.size()>2){
        jSize = ivIdeices[1];
        iSize = ivIdeices[2];
      }else if(ivIdeices.size()>1){
        jSize = ivIdeices[1];
      }
    }
    else{
      jSize = ivIdeices[0];
      if(ivIdeices.size()>1)
        iSize = ivIdeices[0];
    }
    int64_t tempRow = jSize + iSize * JSize;
    int64_t curRow = tempRow % height;
    int64_t curCol =  std::ceil(tempRow / height) * KSize + kSize;
    int64_t col = colStart + curCol;
    int64_t row = rowStart + curRow;
    if((col > colNum-1) || (row > rowNum-1)){
      llvm::outs() << "Col = " << col << ", Row = " << row << "\n";
      llvm::outs() << "KSize, JSize, ISize = (" << KSize << ", " << JSize << ", " << ISize << ")\n";
      llvm::outs() << "kSize, jSize, iSize = (" << kSize << ", " << jSize << ", " << iSize << ")\n";
      llvm::outs() << "colNum, rowNum, colStart, rowStart, height = (" << colNum << ", " << rowNum << ", " << colStart << ", " << rowStart << ", " << height << ")\n";
      return false;
    }
    auto colAttr = builder.getIntegerAttr(indexType, col);
    auto rowAttr = builder.getIntegerAttr(indexType, row);
    auto arrayAttr = builder.getArrayAttr({colAttr, rowAttr});
    callOp->setAttr("col, row", arrayAttr);
    return true;
  }

  bool CorePlacement (ModuleOp mod, StringRef topFuncName) {
    auto builder = OpBuilder(mod);
    int64_t colNum = 50;
    int64_t rowNum = 8;
    int64_t rowStart = 0;
    FuncOp topFunc;
    if(!topFind(mod, topFunc, topFuncName)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }

    // Find the CellOp
    // TODO: Handle Multiple CellOps
    CellOp cellOp = getFirstOpOfType<CellOp>(topFunc.getBody());
    if(!cellOp)
      return true;

    if(!cellOp->getAttr("tripCount"))
      return true;
    
    auto arrayAttr = dyn_cast<ArrayAttr>(cellOp->getAttr("tripCount"));
    int64_t coreNum = 1;
    SmallVector<int64_t, 3> tripCounts;
    for(auto attr : arrayAttr){
      auto intAttr = dyn_cast<IntegerAttr>(attr);
      tripCounts.push_back(intAttr.getInt());
      coreNum *= intAttr.getInt();
    }

    CallOp firstcall = getFirstOpOfType<CallOp>(cellOp.getBody());
    unsigned flow_flag = false;
    if(firstcall->hasAttr("kernel"))
      flow_flag = true;
      
    int64_t height_temp = rowNum;
    int64_t rowPlace = 1;  // row need to be placed
    int64_t KSize = 1;
    int64_t JSize = 1;
    int64_t ISize = 1;
    // Determine the height of the cores
    if(flow_flag){
      KSize = tripCounts[0];
      if(tripCounts.size()>2){
        JSize = tripCounts[1];
        ISize = tripCounts[2];
        height_temp = tripCounts[1];
      }else if(tripCounts.size()>1){
        JSize = tripCounts[1];
        height_temp = tripCounts[1];
      }else{
        height_temp = tripCounts[0];
      }
    }
    else{
      height_temp = tripCounts[0];
      JSize = tripCounts[0];
      if(tripCounts.size()>1)
        ISize = tripCounts[0];
    }
    rowPlace = JSize * ISize;
    int64_t height = std::min(height_temp, rowNum);
    int64_t width = coreNum / height;
    int64_t colStart = std::floor((colNum - width) / 2);
    if(width > colNum)
      return false;

    for (auto callOp : cellOp.getOps<CallOp>()){
      if(!callOp->hasAttr("adf.kernel"))
        continue;
      if(!callOp->hasAttr("ivs"))
        return true;
      auto ivArrayAttr = dyn_cast<ArrayAttr>(callOp->getAttr("ivs"));
      // Check if there are reduction loops need to be placed
      
      SmallVector<int64_t, 3> ivIdeices;
      for(auto attr : ivArrayAttr){
        auto intAttr = dyn_cast<IntegerAttr>(attr);
        ivIdeices.push_back(intAttr.getInt());
      }
      
      auto flag = placementNaive1(builder, callOp, height, colNum, rowNum, 
                                  colStart, rowStart, KSize, JSize, ISize, 
                                  flow_flag, ivIdeices);
      if(!flag)
        return false;
    }

    return true;
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesCorePlacementPass() {
  return std::make_unique<AriesCorePlacement>();
}

} // namespace aries
} // namespace mlir