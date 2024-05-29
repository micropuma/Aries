#ifndef ARIES_CONVERSION_LOWERTOADF_LOWERTOADF_TD
#define ARIES_CONVERSION_LOWERTOADF_LOWERTOADF_TD

include "mlir/Pass/PassBase.td"

def LowerToADF : Pass<"lower-to-adf"> {
  let summary = "Lower to adf dialect";

  let description = [{
    This pass lowers the front-end IRs to adf dialect.
  }];
  let dependentDialects = [
    "mlir::aries::adf::ADFDialect"
  ];
}

#endif  // ARIES_CONVERSION_LOWERTOADF_LOWERTOADF_TD