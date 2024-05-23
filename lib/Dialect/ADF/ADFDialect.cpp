#include "mlir/IR/Builders.h"
#include "mlir/IR/DialectImplementation.h"
#include "llvm/include/llvm/ADT/TypeSwitch.h"
#include "aries/Dialect/ADF/ADFDialect.h"
#include "aries/Dialect/ADF/ADFTypes.h"
#include "aries/Dialect/ADF/ADFOps.h"
#include "aries/Dialect/ADF/ADFAttrs.h"

using namespace mlir;
using namespace aries;
using namespace adf;
using namespace mlir::aries::adf;

#include "aries/Dialect/ADF/ADFDialect.cpp.inc"
#define GET_TYPEDEF_CLASSES
#include "aries/Dialect/ADF/ADFTypes.cpp.inc"
#define GET_ATTRDEF_CLASSES
#include "aries/Dialect/ADF/ADFAttrs.cpp.inc"
#include "aries/Dialect/ADF/ADFEnums.cpp.inc"

void ADFDialect::initialize() {
  addTypes<
#define GET_TYPEDEF_LIST
#include "aries/Dialect/ADF/ADFTypes.cpp.inc"
      >();
  addOperations<
#define GET_OP_LIST
#include "aries/Dialect/ADF/ADFOps.cpp.inc"
      >();  
  addAttributes<
#define GET_ATTRDEF_LIST
#include "aries/Dialect/ADF/ADFAttrs.cpp.inc"
      >();
}