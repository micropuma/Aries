#ifndef ARIES_DIALECT_ARIES_ADF_DIALECT_TD
#define ARIES_DIALECT_ARIES_ADF_DIALECT_TD

include "mlir/IR/OpBase.td"

def ADF_Dialect : Dialect {
  let name = "adf";
  let summary = "A dialect for the AIE dataflow graph (ADF)";
  let description = [{
    The `adf` dialect defines the data movement, physical connection,
    placement of AIE cores. This dialect will be translated into the
    C/C++ graph representation.
  }];

  let cppNamespace = "::mlir::aries::adf";
}

#endif  // ARIES_DIALECT_ARIES_ADF_DIALECT_TD