#ifndef ARIES_DIALECT_ARIES_ADF_TYPES_TD
#define ARIES_DIALECT_ARIES_ADF_TYPES_TD

include "ADFDialect.td"
include "mlir/IR/AttrTypeBase.td"

// Base class for ADF types 
class ADF_Type<string name, string typeMnemonic> : TypeDef<ADF_Dialect, name> {
  let mnemonic = typeMnemonic;
}

def Interface : ADF_Type<"Interface", "interface"> {
  let summary = "interface type";
}

def Kernel : ADF_Type<"Kernel", "kernel"> {
  let summary = "kernel type";
}


#endif  // ARIES_DIALECT_ARIES_ADF_TYPES_TD