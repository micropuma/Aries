include "mlir/IR/OpBase.td"

def ADF_Dialect : Dialect {
  let name = "adf";
  let summary = "A dialect for the AIE dataflow graph (ADF)";
  let description = [{
    The `adf` dialect defines the data movement, pysically connection,
    placement of AIE cores.
  }];

  let cppNamespace = "::mlir::aries::adf";
}