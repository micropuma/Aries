#ifndef ARIES_DIALECT_ARIES_ADF_OPS_H
#define ARIES_DIALECT_ARIES_ADF_OPS_H

#include "aries/Dialect/ADF/ADFDialect.h"
#include "aries/Dialect/ADF/ADFTypes.h"
#include "aries/Dialect/ADF/ADFAttrs.h"
#include "mlir/Interfaces/InferTypeOpInterface.h"
#include "mlir/IR/BuiltinOps.h"     
#include "mlir/IR/BuiltinTypes.h"   
#include "mlir/IR/Dialect.h"        

#define GET_OP_CLASSES
#include "aries/Dialect/ADF/ADFOps.h.inc"

#endif  // ARIES_DIALECT_ARIES_ADF_OPS_H