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

  let arguments = (ins Variadic<InterfaceType>:$input_ports);
  let regions = (region AnyRegion:$region);
  let results = (outs Variadic<InterfaceType>:$output_ports);
  let assemblyFormat = [{
    `(` $input_ports `)` regions attr-dict `:` functional-type($input_ports, results)
    }];
}

def KernelOp : ADF_Op<"kernel", [Pure]> {
  let summary = "ADF Dialcet CallOp";
  let description = [{
    This operation is the Callop function in ADF dialect which creates and calls
    the single kernel runs on an AIE. It takes the arguments with KernelArgType
    as inputs and will also return the KernelArgType outputs. Thus the direction
    information (in or out or inout) can be attached.

    ```mlir
      %out = adf.kernel @kernel(%0, %1) :
              (memref<32x32xf32>, !cascade) -> !cascade
    ```
  }];

  let arguments = (ins FlatSymbolRefAttr:$callee, 
                  Variadic<KernelArgType>:$kernel_inputs);
  let results = (outs Variadic<KernelArgType>:$kernel_outputs);

  let assemblyFormat = [{
    $callee `(` $kernel_inputs `)` attr-dict 
    `:` functional-type($kernel_inputs, results)
  }];
}


// -------------
// Operations to create InterfaceType
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


// -------------
// Operations to create KernelArgType
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

// -------------
// Operations for connect the ADF Graph
// -------------

def ConnectOp : ADF_Op<"connect", []> {
  let summary = "Connect interfaces and kernel arguments";
  let description = [{
    This operation builds up a connection between interfaces and kernel
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


#endif  // ARIES_DIALECT_ARIES_ADF_OPS_TD