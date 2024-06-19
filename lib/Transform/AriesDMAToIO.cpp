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

struct AriesDMAToIO : public AriesDMAToIOBase<AriesDMAToIO> {
public:
  AriesDMAToIO() = default;
  AriesDMAToIO(const AriesOptions &opts) {
    PortType=opts.PortType;
    PortWidth=opts.PortWidth;
  }
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";
  
    if (!DMAToIO(mod,topFuncName))
      signalPassFailure();
  }

private:

  bool DMAToIO (ModuleOp mod, StringRef topFuncName) {
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

std::unique_ptr<Pass> createAriesDMAToIOPass() {
  return std::make_unique<AriesDMAToIO>();
}

std::unique_ptr<Pass> createAriesDMAToIOPass(const AriesOptions &opts) {
  return std::make_unique<AriesDMAToIO>(opts);
}

} // namespace aries
} // namespace mlir