#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "aries/Dialect/ADF/ADFDialect.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/Affine/Analysis/LoopAnalysis.h"

using namespace mlir;
using namespace aries;
using namespace adf;
using namespace mlir::affine;
using namespace mlir::memref;
using namespace func;


namespace {

struct AriesFuncUnroll : public AriesFuncUnrollBase<AriesFuncUnroll> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";    
  
    if (!FuncUnroll(mod,topFuncName))
      signalPassFailure();
  }

private:
  bool FuncUnroll (ModuleOp mod,StringRef topFuncName) {
    auto builder = OpBuilder(mod);
    auto topFunc = *(mod.getOps<FuncOp>().begin());
    bool topFunc_flag = false;
    for (FuncOp func : mod.getOps<FuncOp>()) {
      // Check if the attribute exists in this function.
      if (func->getAttr(topFuncName)){
        topFunc = func;
        topFunc_flag = true;
      }
    }

    if(!topFunc_flag){
      topFunc->emitOpError("Top function not found\n");
      return topFunc_flag;
    }

    
    // Find the CellOp
    // TODO: Handle Multiple CellOps
    CellOp cellOp = getFirstOpOfType<CellOp>(topFunc.getBody());
    if(!cellOp)
      return true;

    SmallVector<AffineForOp, 6> band;
    getNestedLoopBand(cellOp.getBody(), band, true);
    SmallVector<Attribute, 3> tripCountList;
    auto indexType = builder.getIndexType();
    //Start from the innermost loop
    for (auto loop: band) {
      // Keep the tripCount info after unrolling
      auto tripCount = getConstantTripCount(loop);
      if(!tripCount.has_value())
        return false;
      auto tripCountVal = tripCount.value();
      auto tripCountAttr = builder.getIntegerAttr(indexType, tripCountVal);
      tripCountList.push_back(tripCountAttr);
      
      if (loop->getAttr("flow")){
        auto annotateFn = [](unsigned i, Operation *op, OpBuilder builder) {
          auto indexType = builder.getIndexType();
          auto valueAttr = builder.getIntegerAttr(indexType,i);
          if (auto dmaop = dyn_cast<DmaOp>(op)){
            if(auto attr = dmaop->getAttr("write"))
              dmaop->setAttr("write", valueAttr);
            else if(auto attr = dmaop->getAttr("read"))
              dmaop->setAttr("read", valueAttr);
          }
          else if (auto callop = dyn_cast<CallOp>(op)){
            // Mark kernel in the reduction chain to modify the kernel interface 
            callop->setAttr("kernel", valueAttr);
            // Mark each loop iteration info for placement
            if(auto attr = callop->getAttr("ivs")){
              auto arrayAttr = dyn_cast<ArrayAttr>(attr);
              SmallVector<Attribute, 3> newAttrList;
              for (auto oldAttr : arrayAttr){
                newAttrList.push_back(oldAttr);
              }
              newAttrList.push_back(valueAttr);
              auto newArrayAttr = builder.getArrayAttr(newAttrList);
              callop->setAttr("ivs", newArrayAttr);
            }else{
              auto arrayAttr = builder.getArrayAttr({valueAttr});
              callop->setAttr("ivs", arrayAttr);
            }
          }
        };
        if (failed(loopUnrollFull(loop, annotateFn)))
          return false;
      }
      else {
        auto annotateFn = [](unsigned i, Operation *op, OpBuilder builder) {
          auto indexType = builder.getIndexType();
          auto valueAttr = builder.getIntegerAttr(indexType,i);
          if (auto callop = dyn_cast<CallOp>(op)){
            // Mark kernel in the reduction chain to modify the kernel interface 
            // Mark each loop iteration info for placement
            if(auto attr = callop->getAttr("ivs")){
              auto arrayAttr = dyn_cast<ArrayAttr>(attr);
              SmallVector<Attribute, 3> newAttrList;
              for (auto oldAttr : arrayAttr){
                newAttrList.push_back(oldAttr);
              }
              newAttrList.push_back(valueAttr);
              auto newArrayAttr = builder.getArrayAttr(newAttrList);
              callop->setAttr("ivs", newArrayAttr);
            }else{
              auto arrayAttr = builder.getArrayAttr({valueAttr});
              callop->setAttr("ivs", arrayAttr);
            }
          }
        };
        if (failed(loopUnrollFull(loop, annotateFn)))
          return false;
      }
    }

    // Mark the tripCount info for placement
    auto arrayAttr = builder.getArrayAttr(tripCountList);
    cellOp->setAttr("tripCount", arrayAttr);

    llvm::SmallVector<std::pair<StringRef, unsigned>, 4> calleeCounts;
    topFunc.walk([&](CallOp call){
      auto calleeName = call.getCallee();
      bool found = false;
      for (auto &entry : calleeCounts) {
        if (entry.first == calleeName) {
          entry.second++;
          auto valueAttr 
               = builder.getIntegerAttr(builder.getIndexType(), entry.second);
          call->setAttr(calleeName,valueAttr);
          call->setAttr("adf.kernel",builder.getUnitAttr());
          found = true;
          break;
        }
      }
      if (!found) {
        calleeCounts.push_back(std::make_pair(calleeName, 0));
        auto valueAttr = builder.getIntegerAttr(builder.getIndexType(), 0);
        call->setAttr(calleeName,valueAttr);
        call->setAttr("adf.kernel",builder.getUnitAttr());
      }
    });
    return topFunc_flag;
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesFuncUnrollPass() {
  return std::make_unique<AriesFuncUnroll>();
}

} // namespace aries
} // namespace mlir