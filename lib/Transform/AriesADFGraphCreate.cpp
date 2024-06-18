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

struct AriesADFGraphCreate : public AriesADFGraphCreateBase<AriesADFGraphCreate> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";
  
    if (!ADFGraphCreate(mod,topFuncName))
      signalPassFailure();
  }

private:
    
  bool ADFGraphCreate (ModuleOp mod, StringRef topFuncName) {
    auto builder = OpBuilder(mod);
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

std::unique_ptr<Pass> createAriesADFGraphCreatePass() {
  return std::make_unique<AriesADFGraphCreate>();
}

} // namespace aries
} // namespace mlir