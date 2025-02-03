#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Dialect/Affine/Passes.h"
#include "llvm/Support/Debug.h"
#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "aries/Dialect/ADF/ADFDialect.h"

using namespace mlir;
using namespace aries;
using namespace adf;
using namespace mlir::memref;

namespace {

struct AriesBroadcastDetect 
      : public AriesBroadcastDetectBase<AriesBroadcastDetect> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    if (!BroadcastDetect(mod))
      signalPassFailure();
  }

private:
  bool BroadcastDetect (ModuleOp mod) {
    auto builder = OpBuilder(mod);

    return true;
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesBroadcastDetectPass() {
  return std::make_unique<AriesBroadcastDetect>();
}

} // namespace aries
} // namespace mlir