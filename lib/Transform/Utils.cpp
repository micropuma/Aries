#include "aries/Transform/Utils.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/Dialect/Affine/Analysis/LoopAnalysis.h"
#include "mlir/Dialect/Affine/Analysis/AffineAnalysis.h"
#include "mlir/Dialect/Affine/Analysis/AffineStructures.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/IntegerSet.h"
#include "llvm/Support/Debug.h"

using namespace mlir;
using namespace affine;
using namespace func;
using namespace presburger;

namespace mlir {
namespace aries {

/// Unrolls this loop completely.
LogicalResult loopUnrollFull(AffineForOp forOp, function_ref<void(unsigned, Operation *, OpBuilder)> annotateFn) {
  std::optional<uint64_t> mayBeConstantTripCount = mlir::affine::getConstantTripCount(forOp);
  if (mayBeConstantTripCount.has_value()) {
    uint64_t tripCount = *mayBeConstantTripCount;
    if (tripCount == 0)
      return success();
    if (tripCount == 1)
      return promoteIfSingleIteration(forOp);
    return loopUnrollByFactor(forOp, tripCount, annotateFn);
  }
  return failure();
}


// Get all the affine.for loops within the FuncOp and return them in the band
void getLoopBands(FuncOp f, SmallVector<AffineForOp, 6> &band, bool reverse) {
  for (AffineForOp forOp : f.getOps<AffineForOp>()) {
    getPerfectlyNestedLoops(band, forOp);
  }
  if (reverse)
    std::reverse(band.begin(), band.end());
}

/// Built-in Function: Checks whether a loop nest is hyper-rectangular or not.
LogicalResult checkIfHyperRectangular(MutableArrayRef<AffineForOp> input) {
  FlatAffineValueConstraints cst;
  SmallVector<Operation *, 8> ops(input.begin(), input.end());
  // 0-d or 1-d is trivially hyper-rectangular.
  if (input.size() <= 1)
    return success();
  if (failed(getIndexSet(ops, &cst))) {
    llvm::dbgs() << "Index set computation failed!\n";
    return failure();
  }
  if (!cst.isHyperRectangular(0, input.size())) {
    llvm::dbgs()<< "Non-hyperrectangular nests not supported for tiling!\n";
    return failure();
  }
  return success();
}

// Find topFunc and collect the Callee functions
bool topFind(ModuleOp mod, FuncOp& topFunc, StringRef topFuncName){
  bool topFunc_flag = false;
  for (FuncOp func : mod.getOps<FuncOp>()) {
    // Check if the attribute exists in this function.
    if (func->getAttr(topFuncName)){
      topFunc = func;
      topFunc_flag = true;
    }
  }
  return topFunc_flag;
}

// Find the callee function called in the topFunc
bool calleeFind(ModuleOp mod, FuncOp topFunc, FuncOp &calleeFuncOp){
  bool calleeFunc_flag = false;
  SmallVector<FuncOp, 2> CalleeList;
  auto topFuncName = topFunc.getName();
  //Collect the callee function list
  for (FuncOp func : mod.getOps<FuncOp>()) {
    if (func.getName()!= topFuncName)
      CalleeList.push_back(func);
  }

  topFunc.walk([&](CallOp callerFuncOp){
    auto calleeName = callerFuncOp.getCallee();
    for(auto funcOp : CalleeList) {
      if (funcOp.getName() == calleeName) {
        calleeFuncOp = funcOp;
        calleeFunc_flag = true;
        break;
      }
    }
  });
  return calleeFunc_flag;
}

}   // namespace aries
}   // namespace mlir