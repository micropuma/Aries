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

    // 所有的test case都开启了new tiling选项
    // 开启new tiling，是指支持ADF graph的ir表示
    // 因此这一部分没有用
    if(!opts.OptEnableNewTiling){
      // Extract the single kernel design
      pm.addPass(createAriesFuncExtractPass());
      pm.addPass(createAriesLoopSimplifyPass());

      // 下面的关于内存的优化十分经典
      pm.addPass(createAriesMemSubviewPass());
      pm.addPass(createAriesMemHoistPass());
      pm.addPass(createAriesMemCopyPass());
      // Convert to ADF dialect
      pm.addPass(createAriesLowerToADFPass());
      pm.addPass(mlir::createCanonicalizerPass());
    }
    
    // 主要是优化dma数据传输操作
    // Perform global optimizations
    // 找寻loop variable dma依赖，方便后续并行化显示发现不可并行点
    pm.addPass(createAriesDependencyExtractPass());
    // 针对adf.func做函数的loop unroll
    pm.addPass(createAriesFuncUnrollPass());
    // 优化冗余的dma操作
    // ARIES的一个重要优化
    pm.addPass(createAriesLocalDataForwardPass());
    
    // Aries工作的一大亮点：支持fpga的pl端
    if(opts.OptEnablePL){
      pm.addPass(createAriesKernelInterfaceCreatePass());
    }else{
      pm.addPass(createAriesBroadcastDetectPass());
      pm.addPass(createAriesL2BufferCreatePass(opts));
    }
    
    // Perform local optimizations
    // 做一些局部优化
    pm.addPass(createAriesDMAToIOPass(opts));
    pm.addPass(mlir::createCanonicalizerPass());
    pm.addPass(createAriesADFCellCreatePass(opts));

    // 两个最重要的pass：Tile Core的place问题和Tile IO的place问题
    pm.addPass(createAriesCorePlacementPass(opts));
    pm.addPass(createAriesIOPlacementPass(opts));

    // 两个物化pass
    pm.addPass(createAriesGMIOMaterializePass());
    pm.addPass(createAriesPLIOMaterializePass(opts));

    // 各种优化pass，包括数据打包，fpga端dataflow模型构建等
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