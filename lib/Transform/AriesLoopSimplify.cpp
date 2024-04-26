#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/Dialect/Affine/Analysis/LoopAnalysis.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/Support/Debug.h"

using namespace mlir;
using namespace aries;
using namespace mlir::affine;
using namespace mlir::arith;
using namespace func;


namespace {

struct AriesLoopSimplify : public AriesLoopSimplifyBase<AriesLoopSimplify> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";
  
    if (!LoopSimplify(mod,topFuncName))
      signalPassFailure();
  }

private:
  bool LoopSimplify (ModuleOp mod,StringRef topFuncName) {
    auto builder = OpBuilder(mod);
    auto topFunc = *(mod.getOps<FuncOp>().begin());
    FuncOp calleeFuncOp;

    if(!calleeFind(mod, topFunc, topFuncName, calleeFuncOp)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }
    
    if (failed(affineMapElim(mod, topFunc, builder)))
      return false;
    
    return true;
  }

  LogicalResult  affineMapElim(ModuleOp mod, FuncOp calleeFuncOp, OpBuilder builder){    
    

    SmallVector<AffineForOp, 6> band;
    getLoopBands(calleeFuncOp, band);

    if (failed(checkIfHyperRectangular(band))){
      llvm::dbgs() << "Not recutangular nested loops!\n";
      return failure();
    }

    auto innerLoop = band[band.size()-1];
    for (auto forOp: band){
      auto step = forOp.getStep().getLimitedValue();
      auto lowerBound = forOp.getLowerBound();
      auto lowerBoundMp = lowerBound.getMap();
      auto lowerBoundOperands = lowerBound.getOperands();

      // Current only support For loops with single arguments
      auto blockArgs = forOp.getBody()->getArguments();
      if (blockArgs.size()!=1){
        llvm::dbgs() << "The number of block arguments of the forOp doesn't euqal 1!\n" ;
        return failure();
      }
      auto blockArg = blockArgs[0];

      //Construct newAffineMaps using affine.apply and replace the old block 
      //arguments to gurantee correct memref accesses
      auto context = mod.getContext();
      builder.setInsertionPointToStart(innerLoop.getBody());
      AffineExpr newDim = getAffineDimExpr(lowerBoundMp.getNumDims(), context);
      //TODO May need to deal with lowerBoundMp with multi-results
      AffineExpr newExpr = lowerBoundMp.getResult(0) + newDim * step;
      unsigned int dimCount = lowerBoundMp.getNumDims() + 1;
      
      AffineMap newMap = AffineMap::get(dimCount, 0, newExpr, context);
      SmallVector<Value, 2> ApplyOperands;
      for (auto lbOperand : lowerBoundOperands)
        ApplyOperands.push_back(lbOperand);
      ApplyOperands.push_back(blockArg);
      auto applyOp = builder.create<AffineApplyOp>(builder.getUnknownLoc(),newMap,ApplyOperands);
      blockArg.replaceUsesWithIf(applyOp, [&](OpOperand &use) {
        if (dyn_cast<AffineLoadOp>(use.getOwner()) ||
            dyn_cast<AffineStoreOp>(use.getOwner()))
            return true;
        else
            return false;
      });

      //Check if the loops has constant trip count
      //And replace the loop bound with constant value
      //TODO: This should work for recutangular loop nests, but may need to extend
      SmallVector<Value, 4> operands;
      AffineMap map;
      getTripCountMapAndOperands(forOp, &map, &operands);
      if (!map.isSingleConstant()){
        llvm::dbgs() << "Involve loops with non-consant trip count!\n";
        return failure();
      }
      auto tripCount = map.getSingleConstantResult();
      forOp.setConstantLowerBound(0);
      forOp.setConstantUpperBound(tripCount);
      
    }
    return success();
  }

};
} // namespace


namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesLoopSimplifyPass() {
  return std::make_unique<AriesLoopSimplify>();
}

} // namespace aries
} // namespace mlir