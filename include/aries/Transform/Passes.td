#ifndef ARIES_TRANSFORM_PASSES_TD
#define ARIES_TRANSFORM_PASSES_TD

include "mlir/Pass/PassBase.td"

def AriesTiling : Pass<"aries-tiling"> {
  let summary = "Conduct three levels of tiling to the loop nests";
  let description = [{
    This pass conduct multiple levels of loop tiling to the nested for loops.
  }];
  let constructor = "mlir::aries::createAriesTilingPass()";
  let options = [
    ListOption<"TileSizes", "tile-sizes", "unsigned",
               "A list of upto 6 loop tile sizes",
               "llvm::cl::ZeroOrMore">
  ];
}

def AriesFuncExtract : Pass<"aries-func-extract"> {
  let summary = "Extract out the functions for each tile";
  let description = [{
    This pass extracts the function of each tile. The argument of each function
    can be explicted shown in the function call. The memref of each function can
    also be clear with specific subviews.
  }];
  let constructor = "mlir::aries::createAriesFuncExtractPass()";
}

#endif  // ARIES_TRANSFORM_PASSES_TD