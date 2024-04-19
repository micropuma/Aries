#ifndef ARIES_TRANSFORM_PASSES_H
#define ARIES_TRANSFORM_PASSES_H

#include "mlir/Pass/Pass.h"
#include <memory>
#include <vector>

namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesTilingPass();
std::unique_ptr<Pass> createAriesFuncExtractPass();

void registerAriesPasses();

#define GEN_PASS_CLASSES
#include "aries/Transform/Passes.h.inc"

}   // namespace aries
}   // namespace mlir


#endif // ARIES_TRANSFORM_PASSES_H