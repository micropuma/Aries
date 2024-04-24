#ifndef ARIES_TRANSFORM_UTILS_H
#define ARIES_TRANSFORM_UTILS_H


#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"

namespace mlir {
namespace aries {

// Get all the affine.for loops within the FuncOp and return them in the band
void getLoopBands(func::FuncOp f, SmallVector<affine::AffineForOp, 6> &band, bool reverse = false);

}   // namespace aries
}   // namespace mlir


#endif // ARIES_TRANSFORM_UTILS_H