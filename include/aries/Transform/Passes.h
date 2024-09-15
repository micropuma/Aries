#ifndef ARIES_TRANSFORM_PASSES_H
#define ARIES_TRANSFORM_PASSES_H

#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Bufferization/IR/Bufferization.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Dialect.h"
#include "mlir/Pass/Pass.h"
#include "aries/Dialect/ADF/ADFDialect.h"

#include <memory>
#include <vector>

namespace mlir {
class Pass;
} // namespace mlir

namespace mlir {
namespace aries {

struct AriesOptions : public PassPipelineOptions<AriesOptions> {
  /// Configure the tiling pass.
  ListOption<unsigned> OptL1TileSize{
      *this, "l1-tile-sizes", llvm::cl::ZeroOrMore,
      llvm::cl::desc("Specify a list of L1 loop tile sizes")};

  ListOption<unsigned> OptL2TileSize{
      *this, "l2-tile-sizes", llvm::cl::ZeroOrMore,
      llvm::cl::desc("Specify a list of L2 loop tile sizes")};
  
  ListOption<unsigned> OptL3TileSize{
      *this, "l3-tile-sizes", llvm::cl::ZeroOrMore,
      llvm::cl::desc("Specify a list of L3 loop tile sizes")};
  
  /// Configure the DMAToIO pass.
  Option<int64_t> OptCoreAlgo{
      *this, "core-algo", llvm::cl::init(0), llvm::cl::desc(
      "Specify the algorithm for core placement 0:vertical,1:horizontally")};
  
  /// Configure the DMAToIO pass.
  Option<std::string> OptPortType{
      *this, "port-type", llvm::cl::init("PLIO"),
      llvm::cl::desc("Specify the type of the ports(PORT,GMIO,PLIO)")};

  /// Configure the DMAToIO pass.
  Option<int64_t> OptPortWidth{
      *this, "plio-width", llvm::cl::init(32),
      llvm::cl::desc("Specify the plio width of a port in bits (32,64,128)")};
  
  /// Configure the DMAToIO pass.
  Option<int64_t> OptPLIOFreq{
      *this, "plio-freq", llvm::cl::init(250),
      llvm::cl::desc("Specify the plio frequency in MHz")};
  
  /// Configure the DMAToIO pass.
  Option<int64_t> OptPortBurst{
      *this, "gmio-burst", llvm::cl::init(64),
      llvm::cl::desc("Specify the gmio burst in bytes (64,128,256)")};
  
  /// Configure the DMAToIO pass.
  Option<int64_t> OptGMIOBW{
      *this, "gmio-bw", llvm::cl::init(1000),
      llvm::cl::desc("Specify the required gmio bandwidth in MB/s")};
  
  /// Configure the DMAToIO pass.
  Option<int64_t> OptAXIWidth{
      *this, "axi-width", llvm::cl::init(32), llvm::cl::desc(
      "Specify the axi width of the DDR ports in bits (32, 64, 128, ...)")};

  /// Configure the split file pass.
  Option<std::string> OptFileName{
      *this, "inputfile-name", llvm::cl::init("adf_graph_temp.cpp"),
      llvm::cl::desc("Specify the temp file name")};
  
  /// Configure the split file pass.
  Option<std::string> OptPathName{
      *this, "path-name", llvm::cl::init("adf_graph_add"),
      llvm::cl::desc("Specify the path name")};
};

std::unique_ptr<Pass> createAriesTilingPass();
std::unique_ptr<Pass> createAriesTilingPass(const AriesOptions &opts);
std::unique_ptr<Pass> createAriesFuncExtractPass();
std::unique_ptr<Pass> createAriesLoopSimplifyPass();
std::unique_ptr<Pass> createAriesMemSubviewPass();
std::unique_ptr<Pass> createAriesMemHoistPass();
std::unique_ptr<Pass> createAriesMemCopyPass();
std::unique_ptr<Pass> createAriesDependencyExtractPass();
std::unique_ptr<Pass> createAriesFuncUnrollPass();
std::unique_ptr<Pass> createAriesCorePlacementPass();
std::unique_ptr<Pass> createAriesCorePlacementPass(const AriesOptions &opts);
std::unique_ptr<Pass> createAriesLocalDataForwardPass();
std::unique_ptr<Pass> createAriesKernelInterfaceCreatePass();
std::unique_ptr<Pass> createAriesDMAToIOPass();
std::unique_ptr<Pass> createAriesDMAToIOPass(const AriesOptions &opts);
std::unique_ptr<Pass> createAriesADFCellCreatePass();
std::unique_ptr<Pass> createAriesIOPlacementPass();
std::unique_ptr<Pass> createAriesKernelSplitPass();
std::unique_ptr<Pass> createAriesGMIOMaterializePass();
std::unique_ptr<Pass> createAriesPLIOMaterializePass();
std::unique_ptr<Pass> createAriesAXIPackingPass();
std::unique_ptr<Pass> createAriesAXIPackingPass(const AriesOptions &opts);
std::unique_ptr<Pass> createAriesPLDataflowPass();
std::unique_ptr<Pass> createAriesPLDoubleBufferPass();
std::unique_ptr<Pass> createAriesBurstDetectionPass();
std::unique_ptr<Pass> createAriesADFTestPass();
std::unique_ptr<Pass> createAriesFileSplitPass();
std::unique_ptr<Pass> createAriesFileSplitPass(const AriesOptions &opts);

void registerAriesPasses();
void registerAriesPassPipeline();

#define GEN_PASS_CLASSES
#include "aries/Transform/Passes.h.inc"

}   // namespace aries
}   // namespace mlir


#endif // ARIES_TRANSFORM_PASSES_H