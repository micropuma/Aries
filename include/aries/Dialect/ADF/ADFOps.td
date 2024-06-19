#ifndef ARIES_DIALECT_ARIES_ADF_OPS_TD
#define ARIES_DIALECT_ARIES_ADF_OPS_TD

include "ADFDialect.td"
include "ADFTypes.td"

// Base class for ADF operations 
class ADF_Op<string mnemonic, list<Trait> traits = []> :
    Op<ADF_Dialect, mnemonic, traits>;

def GraphOp : ADF_Op<"graph", [Pure, SingleBlock, NoTerminator]> {
  let summary = "An operation presenting an ADF graph";
  let description = [{
    This is an operation to represent an ADF graph.  This operation includes
    the other defined operations in ADF dialect including the GraphOp as well. 

    ```mlir
    %plio_in = adf.plio.create() : !adf.plio<In, 128>
    %gmio_in = adf.gmio.create() : !adf.gmio<In, 128>
    %gmio_out0,%gmio_out1 = adf.graph(%plio_in,%gmio_in){
    
    } : (!adf.plio<In, 128>, !adf.gmio<In, 128>) -> (!adf.gmio<Out, 128>,!adf.gmio<Out, 128>)
    ```
  }];

  let arguments = (ins Variadic<GraphIOType>:$input_ports);
  let regions = (region AnyRegion:$region);
  let results = (outs Variadic<GraphIOType>:$output_ports);
  let assemblyFormat = [{
    `(` $input_ports `)` regions attr-dict `:` functional-type($input_ports, results)
    }];
}

def KernelOp : ADF_Op<"kernel", [Pure]> {
  let summary = "ADF Dialcet CallOp";
  let description = [{
    This operation is the Callop function in ADF dialect which creates and calls
    the single kernel runs on an AIE. It takes the arguments with KernelIOType
    as inputs and will also return the KernelIOType outputs. Thus the direction
    information (in or out or inout) can be attached.

    ```mlir
      %out = adf.kernel @kernel(%0, %1) :
              (memref<32x32xf32>, !cascade) -> !cascade
    ```
  }];

  let arguments = (ins FlatSymbolRefAttr:$callee, 
                  Variadic<KernelIOType>:$kernel_inputs);
  let results = (outs Variadic<KernelIOType>:$kernel_outputs);

  let assemblyFormat = [{
    $callee `(` $kernel_inputs `)` attr-dict 
    `:` functional-type($kernel_inputs, results)
  }];
}


// -------------
// Operations to create GraphIOType
// -------------

def PORTOp : ADF_Op<"port.create", []> {
  let summary = "General port creation";
  let description = [{
    This operation creates a port with direction. This will be
    converted to PLIO or GMIO or as the subgraph in/output.

    ```mlir
      %port0 = adf.port.create() : !adf.port<In>
    ```
  }];

  let results = (outs GMIOType:$port);
  let assemblyFormat = " `(``)` attr-dict `:` type(results)";
}

def PLIOOp : ADF_Op<"plio.create", []> {
  let summary = "PLIO port creation";
  let description = [{
    This operation creates a plio port with port width and direction.

    ```mlir
      %plio0 = adf.plio.create() : !adf.plio<In,128> 
    ```
  }];

  let results = (outs PLIOType:$plio);
  let assemblyFormat = " `(``)` attr-dict `:` type(results)";

}

def GMIOOp : ADF_Op<"gmio.create", []> {
  let summary = "GMIO port creation";
  let description = [{
    This operation creates a gmio port with port width and direction.

    ```mlir
      %gmio0 = adf.gmio.create() : !adf.gmio<In,128>
    ```
  }];

  let results = (outs GMIOType:$gmio);
  let assemblyFormat = " `(``)` attr-dict `:` type(results)";
}


//This may replace the seperated operations above PORTOp, PLIOOp, GMIOOp
def CreateGraphIOOp : ADF_Op<"graph.io", []> {
  let summary = "graph input/output creation";
  let description = [{
    This operation creates either PLIO, GMIO or Port.

    ```mlir
      %plio0 = adf.graph.io(PLIO) : !adf.plio<In,128> 
      %gmio0 = adf.graph.io(GMIO) : !adf.gmio<In,128>
      %port0 = adf.graph.io(PORT) : !adf.port<In>
    ```
  }];

  let arguments = (ins GraphIOName: $name);
  let results = (outs GraphIOType:$graphio);
  let assemblyFormat = " `(` $name `)` attr-dict `:` type(results)";

  let hasCanonicalizer = 1;
}


// -------------
// Operations to create KernelIOType
// -------------

def BufferOp : ADF_Op<"buffer.create", []> {
  let summary = "buffer creation";
  let description = [{
    This operation creates a buffer.

    ```mlir
      %buffer0 = adf.buffer.create() : memref<32x32xi32>
    ```
  }];

  let results = (outs AnyMemRef:$buffer);
  let assemblyFormat = " `(``)` attr-dict `:` type(results)";

}

def StreamOp : ADF_Op<"stream.create", []> {
  let summary = "stream creation";
  let description = [{
    This operation creates a stream.

    ```mlir
      %stream0 = adf.stream.create() : !adf.stream
    ```
  }];

  let results = (outs StreamType:$stream);
  let assemblyFormat = " `(``)` attr-dict `:` type(results)";

}

def CascadeOp : ADF_Op<"cascade.create", []> {
  let summary = "cascade creation";
  let description = [{
    This operation creates a cascade.

    ```mlir
      %cascade0 = adf.cascade.create() : !adf.cascade
    ```
  }];

  let results = (outs CascadeType:$cascade);
  let assemblyFormat = " `(``)` attr-dict `:` type(results)";

}

//This may replace the seperated operations above BufferOp, StreamOp, CascadeOp
def CreateKernelIOOp : ADF_Op<"kernel.io", []> {
  let summary = "kernel input/output creation";
  let description = [{
    This operation creates either buffer, stream or cascade.

    ```mlir
      %buffer0  = adf.kernel.io() : memref<32x32xi32>
      %stream0  = adf.kernel.io() : !adf.stream
      %cascade0 = adf.kernel.io() : !adf.cascade
    ```
  }];

  let arguments = (ins KernelIOName: $name);
  let results = (outs KernelIOType:$kernelarg);
  let assemblyFormat = " `(` $name `)` attr-dict `:` type(results)";
}

// -------------
// Operations for connecting the ADF Graph
// -------------


//This is similar to dma_memcpy_nd in MLIR-AIR
def DmaOp: ADF_Op<"dma", [AttrSizedOperandSegments]> {
  let summary = "This is a dma operator that copy data between memrefs";
  let description = [{
    This is a dma operator that copy data between memrefs
  }];
  let arguments = (
    ins AnyMemRef:$src,
        Variadic<Index>:$src_offsets,
        Variadic<Index>:$src_sizes,
        Variadic<Index>:$src_strides,
        AnyMemRef:$dst,
        Variadic<Index>:$dst_offsets,
        Variadic<Index>:$dst_sizes,
        Variadic<Index>:$dst_strides
  );
  let assemblyFormat = [{
    `(` $src `[` ($src_offsets^)? `]``[` ($src_sizes^)? `]``[` ($src_strides^)? `]` `,`
        $dst `[` ($dst_offsets^)? `]``[` ($dst_sizes^)? `]``[` ($dst_strides^)? `]` `)`  attr-dict `:`
    `(` type($src) `,` type($dst) `)`
  }];
}

def ConnectOp : ADF_Op<"connect", []> {
  let summary = "Connect graph input/output and kernel arguments";
  let description = [{
    This operation builds up a connection between graph input/output and kernel
    arguments. This should be translate to the connect api in adf C/C++.

    ```mlir
      %plio = adf.plio.create() : !adf.plio<In, 128>
      %buffer0 = adf.buffer.create() : memref<32x32xi32>
      adf.connect(%plio, %buffer0) : (!adf.plio<In, 128>, memref<32x32xi32>)
    ```
  }];
  let arguments = (ins ConnectType:$src, ConnectType:$dst);
  let assemblyFormat = " `(` $src `,` $dst `)` attr-dict `:` `(` type($src) `,` type($dst) `)`" ;

}

def IOPushOp: ADF_Op<"io.push", [AttrSizedOperandSegments]> {
  let summary = "Push the externel memory to a graph io";
  let description = [{
    It creates an operation to connect the externel memory 
    (out of aie) with the graph io.

    It pushes an N-dimension of data in one memref to multiple ADF IOs.
    This could be used for broadcast in the DMA side (Note that broadcast in the
    ADF graph side is implictly handled). 

    ```mlir
      %mem0 = memref.alloc() : memref<64x64xf32>
      %plio0 = adf.graph.io(PLIO) : !adf.plio<In,128>
      %plio1 = adf.graph.io(PLIO) : !adf.plio<In,128>  
      adf.io.push(%mem0[0, 0][32, 32][1, 1], %plio0, %plio1) 
      : (memref<64x64xf32>, : !adf.plio<In,128>, !adf.plio<In,128>)
    ```
  }];
  let arguments = (
    ins AnyMemRef:$src,
        Variadic<Index>:$src_offsets,
        Variadic<Index>:$src_sizes,
        Variadic<Index>:$src_strides,
        Variadic<GraphIOType>:$dst
  );
  let assemblyFormat = [{
    `(` $src `[` ($src_offsets^)? `]``[` ($src_sizes^)? `]``[` ($src_strides^)? `]` `,`
        $dst `)`  attr-dict `:` `(` type($src) `,` type($dst) `)`
  }];
  let hasCanonicalizer = 1;
}

def IOPopOp: ADF_Op<"io.pop", [AttrSizedOperandSegments]> {
  let summary = "Pop the data from graph io to the externel memory";
  let description = [{
    It creates an operation to pop the data from graph io to the externel memory 
    (out of aie).

    It pops an N-dimension of data in a memref to an ADF graph IO.

    ```mlir
      %mem0 = memref.alloc() : memref<64x64xf32>
      %plio0 = adf.graph.io(PLIO) : !adf.plio<In,128> 
      adf.io.pop(%plio0,%mem0[0, 0][32, 32][1, 1]) 
      : (!adf.plio<In,128> : memref<64x64xf32>)
    ```
  }];
  let arguments = (
    ins GraphIOType:$src,
        AnyMemRef:$dst,
        Variadic<Index>:$dst_offsets,
        Variadic<Index>:$dst_sizes,
        Variadic<Index>:$dst_strides
  );
  let assemblyFormat = [{
    `(` $src `,` 
    $dst `[` ($dst_offsets^)? `]``[` ($dst_sizes^)? `]``[` ($dst_strides^)? `]`
     `)`  attr-dict `:` `(` type($src) `,` type($dst) `)`
  }];
}


#endif  // ARIES_DIALECT_ARIES_ADF_OPS_TD