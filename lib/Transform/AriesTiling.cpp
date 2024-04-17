#include "aries/Transform/Passes.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/Analysis/Liveness.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"

using namespace mlir;
using namespace mlir::affine;
using namespace aries;
using namespace func;

namespace {

struct AriesTiling : public AriesTilingBase<AriesTiling> {
    void runOnOperation() override {
        auto mod = getOperation();
        unsigned defaultTileSizes = 32;
        SmallVector<unsigned,6> optTileSizes;

        // Receive Specified Tiling Factors
        optTileSizes.clear();
        if (TileSizes.size() > 0) {
            // Initialize tile sizes from the command line.
            for (unsigned i = 0; i < TileSizes.size(); ++i) {
              optTileSizes.push_back(TileSizes[i]);
            }
        }


        mod->walk([&](FuncOp op){
            std::vector<SmallVector<AffineForOp, 6>> bands;
            getTileableBands(op, &bands);
            

            for (auto band : bands) {
                // Set the default tiling fatctor
                SmallVector<unsigned,6> tileSizes(band.size(),defaultTileSizes);

                // Assign received tiling factors to the tilable loop bands
                for (unsigned i = 0; i < std::min(band.size(),optTileSizes.size()); ++i) {
                  tileSizes[i] = optTileSizes[i];
                }
                
                SmallVector<AffineForOp,6> tileBand;

                if (failed(tilePerfectlyNested(band, tileSizes, &tileBand)))
                    return signalPassFailure();
                
            }
        });
        
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