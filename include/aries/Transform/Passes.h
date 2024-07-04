#ifndef ARIES_TRANSFORM_PASSES_H
#define ARIES_TRANSFORM_PASSES_H

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
  ListOption<unsigned> TileSize{
      *this, "tile-sizes", llvm::cl::ZeroOrMore,
      llvm::cl::desc("Specify a list of loop tile sizes")};
  
  /// Configure the DMAToIO pass.
  Option<std::string> OptPortType{
      *this, "port-type", llvm::cl::init("PLIO"),
      llvm::cl::desc("Specify the type of the ports(PORT,GMIO,PLIO)")};

  /// Configure the DMAToIO pass.
  Option<int64_t> OptPortWidth{
      *this, "port-width", llvm::cl::init(128),
      llvm::cl::desc("Specify the port width of a port (32,64,128)")};
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
std::unique_ptr<Pass> createAriesKernelInterfaceCreatePass();
std::unique_ptr<Pass> createAriesLocalDataForwardPass();
std::unique_ptr<Pass> createAriesADFCellCreatePass();
std::unique_ptr<Pass> createAriesADFTestPass();

void registerAriesPasses();
void registerAriesPassPipeline();

#define GEN_PASS_CLASSES
#include "aries/Transform/Passes.h.inc"

}   // namespace aries
}   // namespace mlir


#endif // ARIES_TRANSFORM_PASSES_H