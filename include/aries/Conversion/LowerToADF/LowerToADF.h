#ifndef ARIES_CONVERSION_LOWERTOADF_LOWERTOADF_H
#define ARIES_CONVERSION_LOWERTOADF_LOWERTOADF_H

#include "mlir/Pass/Pass.h"
#include "aries/Dialect/ADF/ADFDialect.h"

namespace mlir {
namespace aries {
namespace adf {

#define GEN_PASS_DECL
#include "aries/Conversion/LowerToADF/LowerToADF.h.inc"

#define GEN_PASS_REGISTRATION
#include "aries/Conversion/LowerToADF/LowerToADF.h.inc"

}  // namespace adf
}  // namespace aries
}  // namespace mlir

#endif // ARIES_CONVERSION_LOWERTOADF_LOWERTOADF_H