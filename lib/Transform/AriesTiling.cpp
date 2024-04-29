#include "aries/Transform/Passes.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"

using namespace mlir;
using namespace mlir::affine;
using namespace aries;
using namespace func;

namespace mlir {
namespace aries {


  bool applyLoopTiling(ModuleOp mod, unsigned defaultTileSizes,SmallVector<unsigned,6> &optTileSizes){
    auto b = OpBuilder(mod);
    
    for (auto &op : mod.getBody()->getOperations()) {
      if (auto func = dyn_cast<FuncOp>(op)) {
        func->setAttr("top_func", b.getUnitAttr());
        std::vector<SmallVector<AffineForOp, 6>> bands;
        getTileableBands(func, &bands);
        
        for (auto band : bands) {
            // Set the default tiling fatctor
            SmallVector<unsigned,6> tileSizes(band.size(),defaultTileSizes);

            // Assign received tiling factors to the tilable loop bands
            for (unsigned i = 0; i < std::min(band.size(),optTileSizes.size()); ++i) {
              tileSizes[i] = optTileSizes[i];
            }
            
            // Call Affine tiling functions for perfectly nested loops
            SmallVector<AffineForOp,6> tileBand;
            if (failed(tilePerfectlyNested(band, tileSizes, &tileBand)))
                return false;
        }
      }
    }

    return true;
  }

} // namespace aries
} // namespace mlir

namespace {

struct AriesTiling : public AriesTilingBase<AriesTiling> {
    AriesTiling() = default;
    AriesTiling(const AriesOptions &opts) {
      for (unsigned i = 0; i < opts.TileSize.size(); ++i) {
        TileSizes=opts.TileSize[i];
      }
    }
    
    void runOnOperation() override {
        auto mod = dyn_cast<ModuleOp>(getOperation());
        unsigned defaultTileSizes = 32;
        
        // Receive Specified Tiling Factors
        SmallVector<unsigned,6> optTileSizes;
        optTileSizes.clear();
        if (TileSizes.size() > 0) {
            // Initialize tile sizes from the command line.
            for (unsigned i = 0; i < TileSizes.size(); ++i) {
              optTileSizes.push_back(TileSizes[i]);
            }
        }

        if(!applyLoopTiling(mod, defaultTileSizes,optTileSizes))
          return signalPassFailure();
    }
};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesTilingPass() {
  return std::make_unique<AriesTiling>();
}

std::unique_ptr<Pass> createAriesTilingPass(const AriesOptions &opts) {
  return std::make_unique<AriesTiling>(opts);
}


} // namespace aries
} // namespace mlir