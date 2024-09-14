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
    ListOption<"L1TileSizes", "l1-tile-sizes", "unsigned",
               "Specify a list of L1 loop tile sizes",
               "llvm::cl::ZeroOrMore">,
    ListOption<"L2TileSizes", "l2-tile-sizes", "unsigned",
               "Specify a list of L2 loop tile sizes",
               "llvm::cl::ZeroOrMore">,
    ListOption<"L3TileSizes", "l3-tile-sizes", "unsigned",
               "Specify a list of L3 loop tile sizes",
               "llvm::cl::ZeroOrMore">
  ];
}

def AriesFuncExtract : Pass<"aries-func-extract"> {
  let summary = "Extract the function of the tile";
  let description = [{
    This pass extracts the operations within the block loop as a function call 
    by using livess analysis and replacing the arguments in the new function.
  }];
  let constructor = "mlir::aries::createAriesFuncExtractPass()";
}

def AriesLoopSimplify : Pass<"aries-loop-simplify"> {
  let summary = "A customized loop normalization pass";
  let description = [{
    This pass simplifies the nested loops in the extracted function from 
    affineMap to constant values by checking its recutangularity and using 
    affine.apply.
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
    This pass unrolls all the block loops to function calls with different 
    arguments.
  }];
  let constructor = "mlir::aries::createAriesFuncUnrollPass()";
}

def AriesCorePlacement : Pass<"aries-core-placement"> {
  let summary = "Place the AIE cores";
  let constructor = "mlir::aries::createAriesCorePlacementPass()";
}

def AriesLocalDataForward : Pass<"aries-local-data-forward"> {
  let summary = "Forward the data from local buffer to another local buffer";
  let description = [{
    This pass detect the opportunities to do data forwarding between local 
    buffers instead of writing to then reading back from memory out of L1. 
    This pass relies on the annotation in AriesFuncUnroll pass.
  }];
  let constructor = "mlir::aries::createAriesLocalDataForwardPass()";
}

def AriesKernelInterfaceCreate : Pass<"aries-kernel-interface-create"> {
  let summary = "Create new kenrel with interface compatable with ADF kernels";
  let description = [{
    This pass creates the new kernel function that returns the buffers be 
    touched. This identifies the direction of the buffer which is required 
    by ADF kernels.
  }];
  let constructor = "mlir::aries::createAriesKernelInterfaceCreatePass()";
  let dependentDialects = ["mlir::memref::MemRefDialect"];
}

def AriesDMAToIO : Pass<"aries-dma-to-io"> {
  let summary = "Convert DmaOp to GraphIOs";
  let description = [{
    This pass converts the DmaOps to the GraphIOs(PORT,GMIO,PLIO).
  }];
  let constructor = "mlir::aries::createAriesDMAToIOPass()";
  let options = [
    Option<"PortType", "port-type", "std::string", /*default=*/"\"PLIO\"", 
           "Specify the type of the ports(PORT,GMIO,PLIO)">,
    Option<"PortWidth", "plio-width", "int64_t", /*default=*/"32",
           "Specify the plio width of a port in bits (32,64,128)">,
    Option<"PLIOFreq", "plio-freq", "int64_t", /*default=*/"250",
           "Specify the plio frequency in MHz">,
    Option<"PortBurst", "gmio-burst", "int64_t", /*default=*/"64",
           "Specify the gmio burst in bytes (64,128,256)">,
    Option<"GMIOBW", "gmio-bw", "int64_t", /*default=*/"1000",
           "Specify the required gmio bandwidth in MB/s">
  ];
}

def AriesADFCellCreate : Pass<"aries-adf-cell-create"> {
  let summary = "Create adf cell with input/output ports";
  let description = [{
    This pass create the adf cell with input arguments and output values.
  }];
  let constructor = "mlir::aries::createAriesADFCellCreatePass()";
}

def AriesKernelSplit : Pass<"aries-kernel-split"> {
  let summary = "Split adf.kernel to another file";
  let constructor = "mlir::aries::createAriesKernelSplitPass()";
}

def AriesGMIOMaterialize : Pass<"aries-gmio-materialize"> {
  let summary = "Materialize IOPush/IOPop Ops with specific DMA";
  let description = [{
    This pass materialize the IOPush/IOPop Ops with specific DMA. For example,
    the GMIO in AIE1 can only do 1d slicing. Thus another DMA is needed in the 
    host memory to do slicing.
  }];
  let constructor = "mlir::aries::createAriesGMIOMaterializePass()";
}

def AriesPLIOMaterialize : Pass<"aries-plio-materialize"> {
  let summary = "Materialize IOPush/IOPop Ops with specific DMA";
  let description = [{
    This pass extract the IOPush/IOPop Ops into pl.dma func, allocate L2 buffer,
    and materialize the IOPush/IOPop Ops with specific DMA.
  }];
  let constructor = "mlir::aries::createAriesPLIOMaterializePass()";
}

def AriesAXIPacking : Pass<"aries-axi-packing"> {
  let summary = "Packing AXI to larger width";
  let constructor = "mlir::aries::createAriesAXIPackingPass()";
}

def AriesPLDataflow : Pass<"aries-pl-dataflow"> {
  let summary = "Create function for each outer loop and apply dataflow pragma";
  let constructor = "mlir::aries::createAriesPLDataflowPass()";
}

def AriesPLDoubleBuffer : Pass<"aries-pl-double-buffer"> {
  let summary = "Apply double buffer for the data mover between PL and AIE";
  let description = [{
    This pass tranverses all the "module" in the adf.pl, extract them to
    separate functions and use double buffer. 
  }];
  let constructor = "mlir::aries::createAriesPLDoubleBufferPass()";
  let dependentDialects = ["mlir::scf::SCFDialect"];
}

def AriesBurstDetection : Pass<"aries-burst-detection"> {
  let summary = "Attempt to merge loops together to increase burst length";
  let description = [{
    This pass tranverses all the "load" and "store" modules in the adf.pl and
    tries to merge loops together to increase burst length. 
  }];
  let constructor = "mlir::aries::createAriesBurstDetectionPass()";
  let dependentDialects = ["mlir::scf::SCFDialect"];
}

def AriesADFTest : Pass<"aries-adf-test"> {
  let summary = "Test ADF Dialect Ops";
  let constructor = "mlir::aries::createAriesADFTestPass()";
  let dependentDialects = ["mlir::aries::adf::ADFDialect"];
}

// This is just a helper function for translation
// may need to be moved to other places
def AriesFileSplit : Pass<"aries-file-split"> {
  let summary = "Split the adf_graph source and header files";
  let constructor = "mlir::aries::createAriesFileSplitPass()";
  let options = [
    Option<"inputfileName", "inputfile-name", "std::string",
           /*default=*/"\"adf_graph_temp.cpp\"", "Specify the temp file name">,
    Option<"pathName", "path-name", "std::string",
           /*default=*/"\"adf_graph_add\"", "Specify the path name">,
  ];
}

#endif  // ARIES_TRANSFORM_PASSES_TD