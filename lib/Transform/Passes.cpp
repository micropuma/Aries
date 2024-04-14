#include "aries/Transform/Passes.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/Passes.h"

namespace {
#define GEN_PASS_REGISTRATION
#include "aries/Transform/Passes.h.inc"
} // namespace


void mlir::aries::registerAriesPasses() {
  ::registerPasses();
}