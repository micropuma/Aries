#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/Support/Debug.h"
#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "aries/Dialect/ADF/ADFDialect.h"

using namespace mlir;
using namespace aries;
using namespace adf;

namespace {

struct AriesKernelInterfaceCreate : public AriesKernelInterfaceCreateBase<AriesKernelInterfaceCreate> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
  
    if (!KernelInterfaceCreate(mod))
      signalPassFailure();
  }

private:
  bool KernelInterfaceCreate (ModuleOp mod) {

    return true;
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesKernelInterfaceCreatePass() {
  return std::make_unique<AriesKernelInterfaceCreate>();
}

} // namespace aries
} // namespace mlir