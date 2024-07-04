#ifndef ARIES_CONVERSION_PASSES_TD
#define ARIES_CONVERSION_PASSES_TD

include "mlir/Pass/PassBase.td"

def AriesLowerToADF : Pass<"aries-lower-to-adf"> {
  let summary = "Lower to adf dialect";

  let description = [{
    This pass lowers the front-end IRs to adf dialect.
  }];
  let dependentDialects = [
    "mlir::aries::adf::ADFDialect",
    "mlir::affine::AffineDialect"
  ];
  let constructor = "mlir::aries::adf::createAriesLowerToADFPass()";
}

def AriesDMAToIO : Pass<"aries-dma-to-io"> {
  let summary = "Convert DmaOp to GraphIOs";
  let description = [{
    This pass converts the DmaOps to the GraphIOs(PORT,GMIO,PLIO).
  }];
  let dependentDialects = [
    "mlir::aries::adf::ADFDialect"
  ];
  let constructor = "mlir::aries::adf::createAriesDMAToIOPass()";
  let options = [
    Option<"PortType", "port-type", "std::string",
           /*default=*/"\"PLIO\"", "Specify the type of the ports(PORT,GMIO,PLIO)">,
    Option<"PortWidth", "port-width", "int64_t", /*default=*/"128",
           "Specify the port width of a port (32,64,128)">
  ];
}

#endif  // ARIES_CONVERSION_PASSES_TD