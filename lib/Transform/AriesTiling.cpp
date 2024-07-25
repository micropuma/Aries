#include "aries/Transform/Passes.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"

using namespace mlir;
using namespace mlir::affine;
using namespace aries;
using namespace func;

namespace {

struct AriesTiling : public AriesTilingBase<AriesTiling> {
public:
  AriesTiling() = default;
  AriesTiling(const AriesOptions &opts) {
    for (unsigned i = 0; i < opts.OptL1TileSize.size(); ++i) {
      L1TileSizes=opts.OptL1TileSize[i];
    }
  }
  
  void runOnOperation() override {
      auto mod = dyn_cast<ModuleOp>(getOperation());
      unsigned defaultTileSizes = 32;
      
      // Receive Specified Tiling Factors
      SmallVector<unsigned,6> optTileSizes;
      optTileSizes.clear();
      if (L1TileSizes.size() > 0) {
          // Initialize tile sizes from the command line.
          for (unsigned i = 0; i < L1TileSizes.size(); ++i) {
            optTileSizes.push_back(L1TileSizes[i]);
          }
      }

      if(!applyLoopTiling(mod, defaultTileSizes,optTileSizes))
        return signalPassFailure();
  }

private:
  bool applyLoopTiling(ModuleOp mod, unsigned defaultTileSizes,SmallVector<unsigned,6> &optTileSizes){
    auto b = OpBuilder(mod);
    auto loc = b.getUnknownLoc();
    for (auto &op : mod.getBody()->getOperations()) {
      if (auto func = dyn_cast<FuncOp>(op)) {
        func->setAttr("top_func", b.getUnitAttr());
        std::vector<SmallVector<AffineForOp, 6>> bands;
        getTileableBands(func, &bands);

        for (auto band : bands) {
            auto bandSize = band.size();
            // Set the default tiling fatctor
            SmallVector<unsigned,6> tileSizes(bandSize,defaultTileSizes);

            // Assign received tiling factors to the tilable loop bands
            for (unsigned i = 0; i < std::min(bandSize,optTileSizes.size()); ++i) {
              tileSizes[i] = optTileSizes[i];
            }
            
            // Call Affine tiling functions for perfectly nested loops
            SmallVector<AffineForOp,6> tileBand;
            if (failed(tilePerfectlyNested(band, tileSizes, &tileBand)))
                return false;
            
            SmallVector<AffineMap, 6> lbMaps;
            SmallVector<AffineMap, 6> ubMaps;
            SmallVector<Value, 6> lbs;
            SmallVector<Value, 6> ubs;
            SmallVector<int64_t, 6> steps;
            auto outerBlockLoop = tileBand[0];

            for (unsigned i=0; i<bandSize; i++){
              auto blockloop = tileBand[i];
              lbMaps.push_back(blockloop.getLowerBoundMap());
              ubMaps.push_back(blockloop.getUpperBoundMap());
              for(auto operand: blockloop.getLowerBoundOperands())
                lbs.push_back(operand);
              for(auto operand: blockloop.getUpperBoundOperands())
                ubs.push_back(operand);
              steps.push_back(blockloop.getStepAsInt());
            }
            b.setInsertionPoint(outerBlockLoop);
            auto parallelOp = b.create<AffineParallelOp>(
              loc, ArrayRef<Type>{}, ArrayRef<arith::AtomicRMWKind>{}, 
              lbMaps, lbs, ubMaps, ubs, steps);
            
            for (unsigned i=0; i<bandSize; i++){
              auto blockloop = tileBand[i];
              blockloop.getBody()->back().erase();
              parallelOp.getBody()->getOperations().splice(
                  parallelOp.getBody()->begin(), blockloop.getBody()->getOperations());
              blockloop.getInductionVar().replaceAllUsesWith(parallelOp.getIVs()[i]);
              blockloop.erase();
            }
        }
      }
    }
    return true;
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