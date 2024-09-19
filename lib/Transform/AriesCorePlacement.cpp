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
  AriesCorePlacement() = default;
  AriesCorePlacement(const AriesOptions &opts) {
    ColNum=opts.OptColNum;
    RowNum=opts.OptRowNum;
    ColOffset=opts.OptColOffset;
    RowOffset=opts.OptRowOffset;
    CoreAlgo=opts.OptCoreAlgo;
  }
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";
  
    if (!CorePlacement(mod,topFuncName))
      signalPassFailure();
  }

private:
  // All the place algorithm here follows the logic below:
  // pid =  (3d cell -> 1d serialization)
  // Height = The number of rows utilized in this placement
  // col = pid % Height
  // row = floor (pid/Height)

  // In this algorithm, the reduction dim is consecutive vertically
  bool placementNaive0(OpBuilder builder, CellOp cellOp, 
                       unsigned colNum, unsigned rowNum, 
                       unsigned colOffset, unsigned rowOffset,
                       unsigned KSize, unsigned JSize, unsigned ISize){
    auto indexType = builder.getIndexType();
    unsigned rowStart = rowOffset;
    // Determine the height and formula for pid
    unsigned height= std::min(KSize, rowNum);
    unsigned flag;
    if(KSize<=rowNum){
      if(rowNum%KSize==0)
        height= rowNum;
      if(ISize < JSize){
        flag = 0;
      }else{
        flag = 1;
      }
    }else{
      if(ISize < JSize){
        flag = 2;
      }else{
        flag = 3;
      }
    }
    unsigned coreNum = KSize * JSize * ISize;
    unsigned colWidth = std::ceil(coreNum/(float)height);
    if(colWidth > colNum)
      return false;
    unsigned colStart = std::floor((colNum - colWidth) / 2.0) + colOffset;

    for (auto callOp : cellOp.getOps<CallOp>()){
      if(!callOp->hasAttr("adf.kernel"))
        continue;
      if(!callOp->hasAttr("ivs"))
        return true;
      auto ivArrayAttr = dyn_cast<ArrayAttr>(callOp->getAttr("ivs"));
      // Check if there are reduction loops need to be placed
      
      SmallVector<unsigned, 3> ivIdeices;
      for(auto attr : ivArrayAttr){
        auto intAttr = dyn_cast<IntegerAttr>(attr);
        ivIdeices.push_back((unsigned)intAttr.getInt());
      }
      unsigned kSize=0;
      unsigned jSize=0;
      unsigned iSize=0;
      if(ivIdeices.size()>3){
        return false;
      }else if(ivIdeices.size()==3){
        kSize = ivIdeices[0];
        jSize = ivIdeices[1];
        iSize = ivIdeices[2];
      }else if(ivIdeices.size()==2){
        if(KSize ==1){
          jSize = ivIdeices[0];
          iSize = ivIdeices[1];
        }else{
          kSize = ivIdeices[0];
          jSize = ivIdeices[1];
        }
      }else if(ivIdeices.size()==1){
        if(KSize ==1){
          jSize = ivIdeices[0];
        }else{
          kSize = ivIdeices[0];
        }
      }else{
        return true;
      }
      unsigned pid=0;
      unsigned index=0;
      if(flag == 0){
        pid = kSize + jSize * KSize + iSize * JSize * KSize;
      }else if(flag == 1){
        pid = kSize + iSize * KSize + jSize * ISize * KSize;
      }else if(flag == 2){
        index = kSize + jSize * KSize + iSize * JSize * KSize; 
      }else if(flag == 3){
        index = kSize + iSize * KSize + jSize * ISize * KSize;
      }
      if(flag == 2 || flag ==3){
        unsigned remi = index % height;
        unsigned quot = std::floor(index / (float)height);
        if(quot%2==0){
          pid = quot * height + remi;
        }else{
          pid = quot * height + (height-1) - remi;
        }
      }
      unsigned col = colStart + std::floor(pid / (float)height);
      unsigned row = rowStart + pid % height;
      if((col > colNum-1) || (row > rowNum-1))
        return false;
      auto colAttr = builder.getIntegerAttr(indexType, col);
      auto rowAttr = builder.getIntegerAttr(indexType, row);
      auto arrayAttr = builder.getArrayAttr({colAttr, rowAttr});
      callOp->setAttr("col, row", arrayAttr);
    }
    return true;
  }

  // In this algorithm, the reduction dim is consecutive horizontally
  // J dim will first be placed vertically
  bool placementNaive1(OpBuilder builder, CellOp cellOp, 
                       unsigned colNum, unsigned rowNum, 
                       unsigned colOffset, unsigned rowOffset,
                       unsigned KSize, unsigned JSize, unsigned ISize){
    auto indexType = builder.getIndexType();
    unsigned rowStart = rowOffset;
    // Determine the height and formula for pid
    unsigned height;
    unsigned flag;
    if(JSize >= ISize){
      flag = 0;
      if(rowNum>JSize && rowNum%JSize==0)
        height = rowNum;
      else
        height = std::min(JSize, rowNum);
    }else{
      flag = 1;
      if(rowNum>ISize && rowNum%ISize==0)
        height = rowNum;
      else
        height = std::min(ISize, rowNum);
    }
    unsigned coreNum = JSize * ISize;
    unsigned colWidth = std::ceil(coreNum/(float)height) * KSize;
    if(colWidth > colNum)
      return false;
    unsigned colStart = std::floor((colNum - colWidth) / 2.0) + colOffset;

    for (auto callOp : cellOp.getOps<CallOp>()){
      if(!callOp->hasAttr("adf.kernel"))
        continue;
      if(!callOp->hasAttr("ivs"))
        return true;
      auto ivArrayAttr = dyn_cast<ArrayAttr>(callOp->getAttr("ivs"));
      // Check if there are reduction loops need to be placed
      
      SmallVector<unsigned, 3> ivIdeices;
      for(auto attr : ivArrayAttr){
        auto intAttr = dyn_cast<IntegerAttr>(attr);
        ivIdeices.push_back((unsigned)intAttr.getInt());
      }
      unsigned kSize=0;
      unsigned jSize=0;
      unsigned iSize=0;
      if(ivIdeices.size()>3){
        return false;
      }else if(ivIdeices.size()==3){
        kSize = ivIdeices[0];
        jSize = ivIdeices[1];
        iSize = ivIdeices[2];
      }else if(ivIdeices.size()==2){
        if(KSize ==1){
          jSize = ivIdeices[0];
          iSize = ivIdeices[1];
        }else{
          kSize = ivIdeices[0];
          jSize = ivIdeices[1];
        }
      }else if(ivIdeices.size()==1){
        if(KSize ==1){
          jSize = ivIdeices[0];
        }else{
          kSize = ivIdeices[0];
        }
      }else{
        return true;
      }
      
      unsigned rowIndex = 0;
      if(flag==1)
        rowIndex = jSize + iSize * JSize;
      else if(flag==0)
        rowIndex = iSize + jSize * ISize;
      unsigned remi = rowIndex % height;
      unsigned quot = std::floor(rowIndex / (float)height);
      unsigned pid = remi + kSize * height + quot * KSize * height;
      unsigned col = colStart + std::floor(pid / (float)height);
      unsigned row = rowStart + pid % height;
      if((col > colNum-1) || (row > rowNum-1)){
        llvm::outs() << "(coreNum, colWidth, colOffset) : " << coreNum << ", " << colWidth << ", " << colOffset << "\n";
        llvm::outs() << "(KSize, JSize, ISize) : " << KSize << ", " << JSize << ", " << ISize << "\n";
        llvm::outs() << "(kSize, jSize, iSize) : " << kSize << ", " << jSize << ", " << iSize << "\n";
        llvm::outs() << "(rowIndex, remi, quot, flag) : " << rowIndex << ", " << remi << ", " << quot << ", " << flag << "\n";
        llvm::outs() << "(pid, col, row, height) : " << pid << ", " << col << ", " << row << ", " << height << "\n";
        return false;
      }
      auto colAttr = builder.getIntegerAttr(indexType, col);
      auto rowAttr = builder.getIntegerAttr(indexType, row);
      auto arrayAttr = builder.getArrayAttr({colAttr, rowAttr});
      callOp->setAttr("col, row", arrayAttr);
    }
    return true;
  }

  bool CorePlacement (ModuleOp mod, StringRef topFuncName) {
    auto builder = OpBuilder(mod);
    unsigned colNum = ColNum;
    unsigned rowNum = RowNum;
    unsigned colOffset = ColOffset;
    unsigned rowOffset = RowOffset;
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
    
    SmallVector<unsigned, 3> tripCounts;
    for(auto attr : arrayAttr){
      auto intAttr = dyn_cast<IntegerAttr>(attr);
      tripCounts.push_back((unsigned)intAttr.getInt());
    }
    // Here we need to identify the KSize, which assume should be innermost
    // but don't care about the order of ISize and JSize
    unsigned KSize = tripCounts[0];
    unsigned JSize = tripCounts[1];
    unsigned ISize = tripCounts[2];

    if(CoreAlgo == 0){
      if(!placementNaive0(builder, cellOp, colNum, rowNum, colOffset, rowOffset,
                          KSize, JSize, ISize))
        return false;
    }
    else{
      if(!placementNaive1(builder, cellOp, colNum, rowNum, colOffset, rowOffset,
                          KSize, JSize, ISize))
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

std::unique_ptr<Pass> createAriesCorePlacementPass(const AriesOptions &opts) {
  return std::make_unique<AriesCorePlacement>(opts);
}

} // namespace aries
} // namespace mlir