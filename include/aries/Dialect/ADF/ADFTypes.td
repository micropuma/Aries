#ifndef ARIES_DIALECT_ARIES_ADF_TYPES_TD
#define ARIES_DIALECT_ARIES_ADF_TYPES_TD

include "ADFDialect.td"
include "mlir/IR/AttrTypeBase.td"
include "mlir/IR/BuiltinTypes.td"


// Base class for ADF types 
class ADF_Type<string name, string typeMnemonic> : TypeDef<ADF_Dialect, name> {
  let mnemonic = typeMnemonic;
}

def InterfaceType : ADF_Type<"Interface", "interface"> {
  let summary = "interface type";
}

def StreamType : ADF_Type<"Stream", "stream"> {
  let summary = "stream type";
}

def CascadeType : ADF_Type<"Cascade", "cascade"> {
  let summary = "cascade type";
}

def AnyScalarType : AnyTypeOf<[Index, AnySignlessInteger, AnyFloat]>;

def KernelArgType : AnyTypeOf<[StreamType, CascadeType, 
                              Builtin_MemRef, AnyScalarType]>;


#endif  // ARIES_DIALECT_ARIES_ADF_TYPES_TD