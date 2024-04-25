#include "aries/Transform/Utils.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
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


}   // namespace aries
}   // namespace mlir