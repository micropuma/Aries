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

def AriesDependencyExtract : Pass<"aries-dependency-extract"> {
  let summary = "Extract loop-carried dependency";
  let description = [{
    This pass extracts the loop-carried dependency for the parallel loops.
    Currently detect the flow dependency and anotate the loop with "flow".
  }];
  let constructor = "mlir::aries::createAriesDependencyExtractPass()";
  let dependentDialects = ["mlir::aries::adf::ADFDialect"];
}

def AriesFuncUnroll : Pass<"aries-func-unroll"> {
  let summary = "Unroll all the block loops as seperated function calls";
  let description = [{
    This pass unrolls all the block loops to function calls with different arguments.
  }];
  let constructor = "mlir::aries::createAriesFuncUnrollPass()";
}

def AriesKernelInterfaceCreate : Pass<"aries-kernel-interface-create"> {
  let summary = "Create new kenrel with interface compatable with ADF kernels";
  let description = [{
    This pass creates the new kernel function that returns the buffers be touched.
    This identifies the direction of the buffer which is required by ADF kernels.
  }];
  let constructor = "mlir::aries::createAriesKernelInterfaceCreatePass()";
}

def AriesLocalDataForward : Pass<"aries-local-data-forward"> {
  let summary = "Forward the data from local buffer to another local buffer";
  let description = [{
    This pass detect the opportunities to do data forwarding between local buffers
    instead of writing to then reading back from memory out of L1. This pass
    should reply on the annotation in AriesFuncUnroll pass.
  }];
  let constructor = "mlir::aries::createAriesLocalDataForwardPass()";
}

def AriesADFGraphCreate : Pass<"aries-adf-graph-create"> {
  let summary = "Create adf graph with input/output ports";
  let description = [{
    This pass create the adf graph with input arguments and output values.
  }];
  let constructor = "mlir::aries::createAriesADFGraphCreatePass()";
}

def AriesADFTest : Pass<"aries-adf-test"> {
  let summary = "Test ADF Dialect Ops";
  let constructor = "mlir::aries::createAriesADFTestPass()";
  let dependentDialects = ["mlir::aries::adf::ADFDialect"];
}

#endif  // ARIES_TRANSFORM_PASSES_TD