#ifndef ARIES_DIALECT_ARIES_ADF_ATTRS_TD
#define ARIES_DIALECT_ARIES_ADF_ATTRS_TD

include "ADFDialect.td"
include "mlir/IR/AttrTypeBase.td"
include "mlir/IR/EnumAttr.td"

// Define ADF Attributes
def DirIn:     I32EnumAttrCase<"In", 0>;
def DirOut:    I32EnumAttrCase<"Out", 1>;
def DirInout:  I32EnumAttrCase<"Inout", 2>;
def Width128:  I32EnumAttrCase<"Width128", 128>;
def Width64:   I32EnumAttrCase<"Width64" , 64>;
def Width32:   I32EnumAttrCase<"Width32" , 32>;

def PortDir: I32EnumAttr<"PortDir", "Direction of the ports",
  [DirIn, DirOut, DirInout]> {
}

def PortWidth: I32EnumAttr<"PortWidth", "Width of the ports",
  [Width128, Width64, Width32]> {
}

#endif  // ARIES_DIALECT_ARIES_ADF_ATTRS_TD