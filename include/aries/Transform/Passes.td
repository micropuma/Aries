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
  let summary = "Extract out the operations within the innermost block loop as a function";
  let description = [{
    This pass extracts the operations within the block loop as a function call by using livess
    analysis and replacing the arguments in the new function.
  }];
  let constructor = "mlir::aries::createAriesFuncExtractPass()";
}

def AriesLoopSimplify : Pass<"aries-loop-simplify"> {
  let summary = "Simplify the boundray of the nested loops in kernel to constant values";
  let description = [{
    This pass simplifies the nested loops in the extracted function from affineMap to constant
    values by checking its recutangularity and using affine.apply.
  }];
  let constructor = "mlir::aries::createAriesLoopSimplifyPass()";
}

def AriesMemSubview : Pass<"aries-memref-subview"> {
  let summary = "Extract Memref subview for each operands";
  let description = [{
    This pass extract the memref subview for every operands in each function call.
    The function call is finally simplified with only memref arguments without 
    other indeices.
  }];
  let constructor = "mlir::aries::createAriesMemSubviewPass()";
}

def AriesFuncUnroll : Pass<"aries-func-unroll"> {
  let summary = "Unroll all the block loops as seperated function calls";
  let description = [{
    This pass unrolls all the block loops to function calls with different arguments.
  }];
  let constructor = "mlir::aries::createAriesFuncUnrollPass()";
}

#endif  // ARIES_TRANSFORM_PASSES_TD