#ifndef ARIES_DIALECT_ARIES_ADF_DIALECT_TD
#define ARIES_DIALECT_ARIES_ADF_DIALECT_TD

include "mlir/IR/OpBase.td"
include "mlir/IR/AttrTypeBase.td"

def ADF_Dialect : Dialect {
  let name = "adf";
  let summary = "A dialect for the AIE dataflow graph (ADF)";
  let description = [{
    The `adf` dialect defines the data movement, physical connection,
    placement of AIE cores. This dialect will be translated into the
    C/C++ graph representation.
  }];

  let cppNamespace = "::mlir::aries::adf";

  let useDefaultTypePrinterParser = 1;
}


// Base class for ADF operations 
class ADF_Op<string mnemonic, list<Trait> traits = []> :
    Op<ADF_Dialect, mnemonic, traits>;

// Base class for ADF types 
class ADF_Type<string name, string typeMnemonic> : TypeDef<ADF_Dialect, name> {
  let mnemonic = typeMnemonic;
}

#endif  // ARIES_DIALECT_ARIES_ADF_DIALECT_TD