#ifndef ARIES_TRANSFORM_UTILS_H
#define ARIES_TRANSFORM_UTILS_H


#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"

using namespace mlir;
using namespace affine;
using namespace func;

namespace mlir {
namespace aries {

// Get all the affine.for loops within the FuncOp and return them in the band
void getLoopBands(FuncOp f, SmallVector<AffineForOp, 6> &band, bool reverse = false);

/// Built-in Function: Checks whether a loop nest is hyper-rectangular or not.
LogicalResult checkIfHyperRectangular(MutableArrayRef<AffineForOp> input);

// Find topFunc and collect the Callee functions
bool topFind(ModuleOp mod, FuncOp& topFunc, StringRef topFuncName);

// Find the callee function called in the topFunc
bool calleeFind(ModuleOp mod, FuncOp topFunc, FuncOp &calleeFuncOp);

}   // namespace aries
}   // namespace mlir


#endif // ARIES_TRANSFORM_UTILS_H