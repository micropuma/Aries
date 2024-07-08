#include "mlir/Tools/mlir-translate/Translation.h"
#include "aries/Translation/Emitter.h"

using namespace mlir;
using namespace aries;
using namespace func;

LogicalResult aries::emitKernelFunc(ModuleOp module, raw_ostream &os) {
  for (auto func : module.getOps<FuncOp>()) {
    os << R"XXX(
//===------------------------------------------------------------*- C++ -*-===//
//
// Automatically generated file for AIE kernel supported by Vitis Flow.
//
//===----------------------------------------------------------------------===//

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include <aie_api/utils.hpp>
#include <adf/io_buffer/io_buffer.h>
using namespace adf;

)XXX";

    if (failed(aries::emitAIEVecToCpp(func, false, os)))
      return failure();
  }

  return success();
}
