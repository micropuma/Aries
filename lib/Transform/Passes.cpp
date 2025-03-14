#include "aries/Transform/Passes.h"
#include "aries/Conversion/Passes.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/Passes.h"

using namespace mlir;
using namespace aries;
using namespace adf;

namespace {
#define GEN_PASS_REGISTRATION
#include "aries/Transform/Passes.h.inc"
} // namespace

// backbone of the pass pipeline
void mlir::aries::registerAriesPassPipeline() {
  PassPipelineRegistration<AriesOptions>(
  "aries-pipeline", "Compile to AIE array",
  [](OpPassManager &pm, const AriesOptions &opts) {

    // Perform multi-level tiling
    pm.addPass(createAriesTilingPass(opts));

    // 默认test case没有开启新的tiling
    if(!opts.OptEnableNewTiling){
      // Extract the single kernel design
      pm.addPass(createAriesFuncExtractPass());
      pm.addPass(createAriesLoopSimplifyPass());
      pm.addPass(createAriesMemSubviewPass());
      pm.addPass(createAriesMemHoistPass());
      pm.addPass(createAriesMemCopyPass());
      // Convert to ADF dialect
      pm.addPass(createAriesLowerToADFPass());
      pm.addPass(mlir::createCanonicalizerPass());
    }
    
    // Perform global optimizations
    pm.addPass(createAriesDependencyExtractPass());
    pm.addPass(createAriesFuncUnrollPass());
    pm.addPass(createAriesLocalDataForwardPass());
    
    if(opts.OptEnablePL){
      pm.addPass(createAriesKernelInterfaceCreatePass());
    }else{
      pm.addPass(createAriesBroadcastDetectPass());
      pm.addPass(createAriesL2BufferCreatePass(opts));
    }
    
    // Perform local optimizations
    pm.addPass(createAriesDMAToIOPass(opts));
    pm.addPass(mlir::createCanonicalizerPass());
    pm.addPass(createAriesADFCellCreatePass(opts));
    pm.addPass(createAriesCorePlacementPass(opts));
    pm.addPass(createAriesIOPlacementPass(opts));
    pm.addPass(createAriesGMIOMaterializePass());
    pm.addPass(createAriesPLIOMaterializePass(opts));
    pm.addPass(createAriesAXIPackingPass(opts));
    pm.addPass(createAriesPLDataflowPass());
    pm.addPass(mlir::createCanonicalizerPass());
    pm.addPass(createAriesBurstDetectionPass());
    pm.addPass(createAriesFuncEliminatePass());
    pm.addPass(createAriesPLDoubleBufferPass());
    pm.addPass(mlir::createCanonicalizerPass());
    if(opts.OptEnableSerial){
      pm.addPass(createAriesPLSerializePass());
      pm.addPass(mlir::createCanonicalizerPass());
    }
  });
}


void mlir::aries::registerAriesPasses() {
  registerPasses();
  registerAriesPassPipeline();
}