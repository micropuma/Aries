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

    
    SmallVector<AffineForOp, 6> bands;
    getLoopBands(topFunc, bands, true);
    
    //Start from the innermost band loop
    for (auto band: bands) {
      if (band->getAttr("flow")){
        auto annotateFn = [](unsigned i, Operation *op, OpBuilder builder) {
          if (auto dmaop = dyn_cast<DmaOp>(op)){
            if(auto attr = dmaop->getAttr("write")){
              auto valueAttr = builder.getIntegerAttr(builder.getIndexType(), i);
              dmaop->setAttr("write", valueAttr);
            }else if(auto attr = dmaop->getAttr("read")){
              auto valueAttr = builder.getIntegerAttr(builder.getIndexType(), i);
              dmaop->setAttr("read", valueAttr);
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

    llvm::SmallVector<std::pair<StringRef, unsigned>, 4> calleeCounts;
    topFunc.walk([&](CallOp call){
      auto calleeName = call.getCallee();
      bool found = false;
      for (auto &entry : calleeCounts) {
        if (entry.first == calleeName) {
          entry.second++;
          auto valueAttr = builder.getIntegerAttr(builder.getIndexType(), entry.second);
          call->setAttr(calleeName,valueAttr);
          found = true;
          break;
        }
      }
      if (!found) {
        calleeCounts.push_back(std::make_pair(calleeName, 0));
        auto valueAttr = builder.getIntegerAttr(builder.getIndexType(), 0);
        call->setAttr(calleeName,valueAttr);
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