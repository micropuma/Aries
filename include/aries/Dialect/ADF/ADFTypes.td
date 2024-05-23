#ifndef ARIES_DIALECT_ARIES_ADF_TYPES_TD
#define ARIES_DIALECT_ARIES_ADF_TYPES_TD

include "ADFDialect.td"
include "mlir/IR/BuiltinTypes.td"
include "mlir/IR/CommonTypeConstraints.td"


def PortType : ADF_Type<"Port", "port"> {
  let summary = "port type";
  let parameters = (ins "Attribute":$dir);
  let assemblyFormat = "`<` $dir `>` ";
}

def PLIOType : ADF_Type<"PLIO", "plio"> {
  let summary = "plio type";
  let parameters = (ins "Attribute":$dir, AnySignlessInteger:$width);
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
                              AnyMemRef, AnyScalarType]>;


#endif  // ARIES_DIALECT_ARIES_ADF_TYPES_TD