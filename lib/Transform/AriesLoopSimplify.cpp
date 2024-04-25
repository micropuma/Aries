#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/Dialect/Affine/Analysis/LoopAnalysis.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "llvm/ADT/BitVector.h"
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

  bool calleeFind(ModuleOp mod, FuncOp topFunc, StringRef topFuncName, FuncOp &calleeFuncOp){
    bool topFunc_flag = false;
    SmallVector<FuncOp, 2> CalleeList;
    //Find topFunc and collect the Callee functions
    for (FuncOp func : mod.getOps<FuncOp>()) {
      // Check if the attribute exists in this function.
      if (func->getAttr(topFuncName)){
        topFunc = func;
        topFunc_flag = true;
      }
      else{
        CalleeList.push_back(func);
      }
    }

    topFunc.walk([&](CallOp callerFuncOp){
      auto calleeName = callerFuncOp.getCallee().str();
      for(auto funcOp : CalleeList) {
        if (funcOp.getName() == calleeName) {
          calleeFuncOp = funcOp;
          break;
        }
      }
    });
    return topFunc_flag;
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
      auto upperBound = forOp.getUpperBound();
      auto upperBoundOperands = upperBound.getOperands();
      // Current only support For loops with single arguments
      if (upperBoundOperands.size()!=1){
        llvm::dbgs() << "The number of loop bound operands doesn't euqal 1!\n" ;
        return failure();
      }
      auto ubOperand = upperBoundOperands[0];

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
      AffineExpr d0 = getAffineDimExpr(0, context);
      AffineExpr d1 = getAffineDimExpr(1, context);
      
      //TODO The newMap is not general enough, it should be constructed based
      //on the original map for ubOperand
      AffineMap newMap = AffineMap::get(2, 0, {d0 + d1}, context);
      SmallVector<Value, 2> ApplyOperands;
      ApplyOperands.push_back(blockArg);
      ApplyOperands.push_back(ubOperand);
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