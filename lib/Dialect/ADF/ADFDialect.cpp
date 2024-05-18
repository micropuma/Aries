#include "mlir/IR/Builders.h"
#include "mlir/IR/DialectImplementation.h"
#include "aries/Dialect/ADF/ADFDialect.h"

using namespace mlir;
using namespace aries;
using namespace adf;
using namespace mlir::aries::adf;

#include "aries/Dialect/ADF/ADFDialect.cpp.inc"

void ADFDialect::initialize() {
}