#include "aries/Transform/Passes.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/Passes.h"

using namespace mlir;
using namespace aries;

namespace {
#define GEN_PASS_REGISTRATION
#include "aries/Transform/Passes.h.inc"
} // namespace

void mlir::aries::registerAriesPassPipeline() {
  PassPipelineRegistration<AriesOptions>(
  "aries-pipeline", "Compile to AIE array",
  [](OpPassManager &pm, const AriesOptions &opts) {
    pm.addPass(createAriesTilingPass(opts));
    pm.addPass(createAriesFuncExtractPass());
    pm.addPass(createAriesLoopSimplifyPass());
    pm.addPass(createAriesMemSubviewPass());
    pm.addPass(createAriesMemHoistPass());
    pm.addPass(createAriesMemCopyPass());
    pm.addPass(createAriesFuncUnrollPass());
    pm.addPass(createAriesDependencyExtractPass());
    pm.addPass(createAriesADFTestPass());
  });
}


void mlir::aries::registerAriesPasses() {
  registerPasses();
  registerAriesPassPipeline();
}