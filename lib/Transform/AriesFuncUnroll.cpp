#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/Affine/Analysis/LoopAnalysis.h"

using namespace mlir;
using namespace aries;
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

    //Start from the innermost loop
    SmallVector<AffineForOp, 6> bands;
    getLoopBands(topFunc, bands, true);
    for (auto band: bands) {
      if (band->getAttr("flow")){
        auto annotateFn = [](unsigned i, Operation *op, OpBuilder builder) {
          if (auto copyop = dyn_cast<CopyOp>(op)){
            if(auto attr = copyop->getAttr("write")){
              auto valueAttr = builder.getIntegerAttr(builder.getIndexType(), i);
              copyop->setAttr("write", valueAttr);
            }else if(auto attr = copyop->getAttr("read")){
              auto valueAttr = builder.getIntegerAttr(builder.getIndexType(), i);
              copyop->setAttr("read", valueAttr);
            }
          }
        };
        if (failed(loopUnrollFull(band,annotateFn)))
          return false;
      }
      else {
        if (failed(loopUnrollFull(band)))
          return false;
      }
      
    }
    

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