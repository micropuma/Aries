#ifndef ARIES_DIALECT_ARIES_ADF_OPS_TD
#define ARIES_DIALECT_ARIES_ADF_OPS_TD

include "ADFDialect.td"
include "ADFAttrs.td"
include "ADFTypes.td"

include "mlir/IR/OpBase.td"
include "mlir/IR/SymbolInterfaces.td"
include "mlir/Interfaces/SideEffectInterfaces.td"
include "mlir/Interfaces/CallInterfaces.td"


def GraphOp : ADF_Op<"graph", [Pure, SingleBlock, NoTerminator]> {
  let summary = "An operation presenting an ADF graph";
  let description = [{
    This is an operation to represent an ADF graph.  This operation includes
    the other defined operations in ADF dialect including the GraphOp as well. 

    ```mlir
    adf.graph(input_plio0,input_plio1,output_plio0) attributes {name = "name"} {
      adf_operations
    }
    ```
  }];

  let arguments = (ins Variadic<InterfaceType>:$input_ports);
  let regions = (region AnyRegion:$region);
  let results = (outs Variadic<InterfaceType>:$output_ports);
  // let assemblyFormat = [{
  //   `(` $input_ports `)` regions attr-dict `:` type($output_ports)
  //   }];
}

def KernelOp : ADF_Op<"kernel", [CallOpInterface,
     DeclareOpInterfaceMethods<SymbolUserOpInterface>]> {
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

  // let assemblyFormat = [{
  //   $callee `(` $kernel_inputs `)` attr-dict 
  //   `:` functional-type($kernel_inputs, $kernel_outputs)
  // }];
}

def PLIOOp : ADF_Op<"plio", [Pure]> {
  let summary = "PLIO port creation";
  let description = [{
    This operation creates a plio port with port width and direction.

    ```mlir
      %plio0 = adf.plio() : !plio<"In",128> 
    ```
  }];

  let results = (outs PLIOType:$plio);
  // let assemblyFormat = "attr-dict `:` type($plio)";

}




#endif  // ARIES_DIALECT_ARIES_ADF_OPS_TD