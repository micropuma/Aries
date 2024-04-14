#include "aries/Transform/Passes.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/Analysis/Liveness.h"

using namespace mlir;
using namespace aries;

namespace {
struct AriesTiling : public AriesTilingBase<AriesTiling> {
    void runOnOperation() override {
        auto mod = getOperation();
        if (0) {
          signalPassFailure();
        }
    }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesTilingPass() {
  return std::make_unique<AriesTiling>();
}

} // namespace aries
} // namespace mlir