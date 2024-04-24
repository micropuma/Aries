#include "aries/Transform/Utils.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/LoopUtils.h"


namespace mlir {
namespace aries {

// Get all the affine.for loops within the FuncOp and return them in the band
void getLoopBands(func::FuncOp f, SmallVector<affine::AffineForOp, 6> &band, bool reverse) {
  for (affine::AffineForOp forOp : f.getOps<affine::AffineForOp>()) {
    getPerfectlyNestedLoops(band, forOp);
  }
  if (reverse)
    std::reverse(band.begin(), band.end());
}

}   // namespace aries
}   // namespace mlir