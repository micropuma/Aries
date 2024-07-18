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
    Option<"PortWidth", "plio-width", "int64_t", /*default=*/"128",
           "Specify the plio width of a port in bits (32,64,128)">,
    Option<"PLIOFreq", "plio-freq", "int64_t", /*default=*/"250",
           "Specify the plio frequency in MHz">,
    Option<"PortBurst", "gmio-burst", "int64_t", /*default=*/"64",
           "Specify the gmio burst in bytes (64,128,256)">,
    Option<"GMIOBW", "gmio-bw", "int64_t", /*default=*/"1000",
           "Specify the required gmio bandwidth in MB/s">
  ];
}

#endif  // ARIES_CONVERSION_PASSES_TD