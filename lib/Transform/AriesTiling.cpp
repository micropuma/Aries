#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Affine/Utils.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/Passes.h"
#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"

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
      L2TileSizes=opts.OptL2TileSize[i];
      L3TileSizes=opts.OptL3TileSize[i];
    }
  }
  
  void runOnOperation() override {
      auto mod = dyn_cast<ModuleOp>(getOperation());
      unsigned defaultTileSizes = 32;

      if(!applyLoopTiling(mod, defaultTileSizes))
        return signalPassFailure();
  }

private:
  bool applyLoopTiling(ModuleOp mod, unsigned defaultTileSizes){
    auto builder = OpBuilder(mod);
    auto loc = builder.getUnknownLoc();
    // TODO: Handle multiple funcs in the module
    for (auto func : mod.getOps<FuncOp>()) {
      func->setAttr("top_func", builder.getUnitAttr());
      SmallVector<AffineForOp, 6> band;
      getLoopBands(func, band);

      auto bandSize = band.size();
      // Set the default tiling fatctor
      SmallVector<unsigned,6> L1tileSizes(bandSize,defaultTileSizes);
      SmallVector<unsigned,6> L2tileSizes(bandSize,defaultTileSizes);
      SmallVector<unsigned,6> L3tileSizes(bandSize,defaultTileSizes);
      // Assign received tiling factors to the tilable loop bands
      for (unsigned i = 0; i < std::min(bandSize,L1TileSizes.size()); ++i)
        L1tileSizes[i] = L1TileSizes[i];
      
      // Call Affine tiling functions for perfectly nested loops
      SmallVector<AffineForOp,6> L1tileBand;
      SmallVector<AffineForOp,6> L2tileBand;
      SmallVector<AffineForOp,6> L3tileBand;
      if (failed(tilePerfectlyNested(band, L1tileSizes, &L1tileBand)))
          return false;
      // L2 tiling if specified
      if(L2TileSizes.size()){
        for (unsigned i = 0; i <std::min(bandSize,L2TileSizes.size());++i)
          L2tileSizes[i] = L2TileSizes[i];
        
        SmallVector<AffineForOp, 6> blockL1tileBand(
          L1tileBand.begin(), L1tileBand.begin() + bandSize);
        
        if (failed(tilePerfectlyNested(
                              blockL1tileBand, L2tileSizes, &L2tileBand)))
            return false;
        
        // L3 tiling if specified
        if(L3TileSizes.size()){
          for (unsigned i = 0; i <std::min(bandSize,L3TileSizes.size());++i)
            L3tileSizes[i] = L3TileSizes[i];
          SmallVector<AffineForOp, 6> blocktileBandL2(
            L2tileBand.begin(), L2tileBand.begin() + bandSize);
          if (failed(tilePerfectlyNested(
                                blocktileBandL2, L3tileSizes, &L3tileBand)))
              return false;
        }
      }
      
      // Replace the affine.for Ops to affine.parallel Ops according to
      // the level of tiling
      auto outerBlockLoop = L1tileBand[0];
      if(L2TileSizes.size()){
        outerBlockLoop = L2tileBand[bandSize];
      }
      // Create nested parallel loops
      builder.setInsertionPoint(outerBlockLoop);
      SmallVector<AffineParallelOp, 6U> parallelops;
      AffineParallelOp innerparallelop;
      for (unsigned i=0; i<bandSize; i++){
        auto blockloop = L1tileBand[i];
        if(L2TileSizes.size()){
          blockloop = L2tileBand[bandSize+i];
        }
        AffineMap lbMap = blockloop.getLowerBoundMap();
        AffineMap ubMap = blockloop.getUpperBoundMap();
        SmallVector<Value, 6> lbs;
        SmallVector<Value, 6> ubs;
        for(auto operand: blockloop.getLowerBoundOperands())
          lbs.push_back(operand);
        for(auto operand: blockloop.getUpperBoundOperands())
          ubs.push_back(operand);
        auto step = blockloop.getStepAsInt();
        auto parallelOp = builder.create<AffineParallelOp>(
          loc, ArrayRef<Type>{}, ArrayRef<arith::AtomicRMWKind>{}, 
          lbMap, lbs, ubMap, ubs, step);
        builder.setInsertionPointToStart(parallelOp.getBody());
        parallelops.push_back(parallelOp);
        innerparallelop = parallelOp;
      }
      // Move the operation from original loops to the nested parallel loops
      for (unsigned i=0; i<bandSize; i++){
        auto blockloop = L1tileBand[i];
        if(L2TileSizes.size()){
          blockloop = L2tileBand[bandSize+i];
        }
        blockloop.getBody()->back().erase();
        innerparallelop.getBody()->getOperations().splice(
                                    innerparallelop.getBody()->begin(), 
                                    blockloop.getBody()->getOperations());
        blockloop.getInductionVar().replaceAllUsesWith(
                                    parallelops[i].getIVs()[0]);
        blockloop.erase();
      }
      // Normalize the affine.parallel Ops
      for(auto parallelop: parallelops){
        normalizeAffineParallel(parallelop);
      }

      PassManager pm(&getContext());
      pm.addPass(createCanonicalizerPass());
      pm.addPass(createCSEPass());
      if (failed(pm.run(func))) {
        return false;
      }

      // Merge the nested parallelOps to a single parallelOp
      SmallVector<AffineMap, 6> lbMaps;
      SmallVector<AffineMap, 6> ubMaps;
      SmallVector<Value, 6> lbs;
      SmallVector<Value, 6> ubs;
      SmallVector<int64_t, 6> steps;
      auto outerBlockParallelLoop = parallelops[0];

      for (unsigned i=0; i<bandSize; i++){
        auto blockParallelloop = parallelops[i];
        if(!blockParallelloop.getLowerBoundsMap().isSingleConstant()||
           !blockParallelloop.getUpperBoundsMap().isSingleConstant())
          return false;
        lbMaps.push_back(blockParallelloop.getLowerBoundsMap());
        ubMaps.push_back(blockParallelloop.getUpperBoundsMap());
        for(auto operand: blockParallelloop.getLowerBoundsOperands())
          lbs.push_back(operand);
        for(auto operand: blockParallelloop.getUpperBoundsOperands())
          ubs.push_back(operand);
        steps.push_back(blockParallelloop.getSteps()[0]);
      }
      builder.setInsertionPoint(outerBlockParallelLoop);
      auto parallelOp = builder.create<AffineParallelOp>(
        loc, ArrayRef<Type>{}, ArrayRef<arith::AtomicRMWKind>{}, 
        lbMaps, lbs, ubMaps, ubs, steps);
      
      for (unsigned i=0; i<bandSize; i++){
        auto blockParallelloop = parallelops[i];
        blockParallelloop.getBody()->back().erase();
        parallelOp.getBody()->getOperations().splice(
            parallelOp.getBody()->begin(), 
            blockParallelloop.getBody()->getOperations());
        blockParallelloop.getBody()->getArgument(0).replaceAllUsesWith(
                                                        parallelOp.getIVs()[i]);
        blockParallelloop.erase();
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