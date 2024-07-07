#include "mlir/Tools/mlir-translate/Translation.h"
#include "aries/Translation/Emitter.h"
#include "aie/Targets/AIETargets.h"


using namespace mlir;
using namespace aries;
using namespace func;

LogicalResult aries::emitKernelFunc(ModuleOp module, raw_ostream &os) {
  for (auto func : module.getOps<FuncOp>()) {
    os << R"XXX(
//===------------------------------------------------------------*- C++ -*-===//
//
// Automatically generated file for MLIR-AIE AIE kernel.
//
//===----------------------------------------------------------------------===//

// #define __AIENGINE__ 1
// #define NOCPP

// #include <cardano.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" {

)XXX";

    if (failed(xilinx::aievec::translateAIEVecToCpp(func, false, os)))
      return failure();

    os << "}\n";
  }

  return success();
}
