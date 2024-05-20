#ifndef ARIES_DIALECT_ARIES_ADF_OPS_TD
#define ARIES_DIALECT_ARIES_ADF_OPS_TD

include "ADFDialect.td"
include "ADFTypes.td"

include "mlir/IR/OpBase.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

// Base class for ADF operations 
class ADF_Op<string mnemonic, list<Trait> traits = []> :
    Op<ADF_Dialect, mnemonic, traits>;

def GraphOp : ADF_Op<"graph", [Pure, SingleBlock, NoTerminator]> {
  let summary = "An operation presenting an ADF graph";
  let description = [{
    This is an operation to represent an ADF graph.  This operation includes
    the other defined operations in ADF dialect including the GraphOp as well. 

    ```mlir
    adf.graph("name") {
      adf_operations
    }
    ```
  }];

  let arguments = (ins StrAttr:$name);
  let regions = (region AnyRegion:$region);
  let assemblyFormat = "`(` $name `)` regions attr-dict";
}


#endif  // ARIES_DIALECT_ARIES_ADF_OPS_TD