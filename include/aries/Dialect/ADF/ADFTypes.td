#ifndef ARIES_DIALECT_ARIES_ADF_TYPES_TD
#define ARIES_DIALECT_ARIES_ADF_TYPES_TD

include "ADFDialect.td"
include "ADFAttrs.td"

// Base class for ADF types 
class ADF_Type<string name, string typeMnemonic> : TypeDef<ADF_Dialect, name> {
  let mnemonic = typeMnemonic;
}

def PortType : ADF_Type<"Port", "port"> {
  let summary = "port type";
  let parameters = (ins PortDirParam:$dir);
  let assemblyFormat = "`<` $dir `>` ";
}

def PLIOType : ADF_Type<"PLIO", "plio"> {
  let summary = "plio type";
  let parameters = (ins "Attribute":$dir, "int":$width);
  let assemblyFormat = "`<` $dir `,` $width `>`";
}

def GMIOType : ADF_Type<"GMIO", "gmio"> {
  let summary = "gmio type";
}

def InterfaceType : AnyTypeOf<[PLIOType, GMIOType, PortType]>;

def StreamType : ADF_Type<"Stream", "stream"> {
  let summary = "stream type";
}

def CascadeType : ADF_Type<"Cascade", "cascade"> {
  let summary = "cascade type";
}

def AnyScalarType : AnyTypeOf<[Index, AnySignlessInteger, AnyFloat]>;

def KernelArgType : AnyTypeOf<[StreamType, CascadeType, 
                              AnyMemRef]>;


#endif  // ARIES_DIALECT_ARIES_ADF_TYPES_TD