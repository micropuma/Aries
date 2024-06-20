#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "llvm/Support/Debug.h"
#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"

using namespace mlir;
using namespace aries;
using namespace mlir::func;

namespace {

struct AriesLocalDataForward : public AriesLocalDataForwardBase<AriesLocalDataForward> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";
  
    if (!LocalDataForward(mod,topFuncName))
      signalPassFailure();
  }

private:

  bool LocalDataForward (ModuleOp mod, StringRef topFuncName) {
    FuncOp topFunc;
    if(!topFind(mod, topFunc, topFuncName)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }

    return true;
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesLocalDataForwardPass() {
  return std::make_unique<AriesLocalDataForward>();
}

} // namespace aries
} // namespace mlir