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
               "Specify a list of loop tile sizes",
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

def AriesMemSubview : Pass<"aries-mem-subview"> {
  let summary = "Extract Memref subview for each operands";
  let description = [{
    This pass extract the memref subview for every memref operands in the callee
    function call.
  }];
  let constructor = "mlir::aries::createAriesMemSubviewPass()";
  let dependentDialects = ["mlir::memref::MemRefDialect"];
}

def AriesMemHoist : Pass<"aries-mem-hoist"> {
  let summary = "Hoist the memref subview to the top function";
  let description = [{
    This pass hoists the memref subview to the top function and erase non-mem
    arguments of the kernel function.
  }];
  let constructor = "mlir::aries::createAriesMemHoistPass()";
}

def AriesMemCopy : Pass<"aries-mem-copy"> {
  let summary = "Allocate new mem and copy the subview to it";
  let description = [{
    This pass allocates new buffers which the memref subview is copied to. 
    This pass aims to represents the dma operation from other memory hierarchy 
    to the local memory of a kernel.
  }];
  let constructor = "mlir::aries::createAriesMemCopyPass()";
}

def AriesFuncUnroll : Pass<"aries-func-unroll"> {
  let summary = "Unroll all the block loops as seperated function calls";
  let description = [{
    This pass unrolls all the block loops to function calls with different arguments.
  }];
  let constructor = "mlir::aries::createAriesFuncUnrollPass()";
}

def AriesDependencyExtract : Pass<"aries-dependency-extract"> {
  let summary = "Extract carry dependency in the adf graph";
  let description = [{
    This pass extracts the carry dependency (pop-push) in the adf graph level.
    It converts the pop-push pair to the direct connection between buffers.
  }];
  let constructor = "mlir::aries::createAriesDependencyExtractPass()";
  let dependentDialects = ["mlir::aries::adf::ADFDialect"];
}

def AriesADFTest : Pass<"aries-adf-test"> {
  let summary = "Test ADF Dialect Ops";
  let constructor = "mlir::aries::createAriesADFTestPass()";
  let dependentDialects = ["mlir::aries::adf::ADFDialect"];
}

#endif  // ARIES_TRANSFORM_PASSES_TD