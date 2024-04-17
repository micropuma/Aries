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

#endif  // ARIES_TRANSFORM_PASSES_TD