#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "llvm/Support/Debug.h"
#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "aries/Dialect/ADF/ADFDialect.h"

using namespace mlir;
using namespace aries;
using namespace adf;
using namespace mlir::func;

namespace {

struct AriesADFGraphCreate : public AriesADFGraphCreateBase<AriesADFGraphCreate> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";
  
    if (!ADFGraphCreate(mod,topFuncName))
      signalPassFailure();
  }

private:
  void OpCollect(FuncOp topFunc, SmallVector<Operation*> &GraphOps, 
                 SmallVector<IOPopOp> &IOPopOps, SmallVector<Value> &ArgIns,
                 SmallVector<Value> &ArgOuts){
    topFunc.walk([&](Operation *op){
      if( dyn_cast<BufferOp>(op) || dyn_cast<ConnectOp>(op) || 
          dyn_cast<CallOp>(op)   || dyn_cast<SetIOWidthOp>(op)){
            GraphOps.push_back(op);
      }else if(auto graphio = dyn_cast<CreateGraphIOOp>(op)){
        PortDir portDir = PortDir::In;
        if(auto type = dyn_cast<PLIOType>(graphio.getResult().getType())){
          portDir = type.getDir();
        }else if(auto type = dyn_cast<GMIOType>(graphio.getResult().getType())){
          portDir = type.getDir();
        }else{
          auto type1 = dyn_cast<PortType>(graphio.getResult().getType());
          portDir = type1.getDir();
        }
        if(portDir == PortDir::In){
          ArgIns.push_back(graphio.getResult());
        }else if(portDir == PortDir::Out){
          GraphOps.push_back(op);
          ArgOuts.push_back(graphio.getResult());
        }
      }else if(auto iopopOp = dyn_cast<IOPopOp>(op)){
        IOPopOps.push_back(iopopOp);
      }
    });
  }

  bool ADFGraphCreate (ModuleOp mod, StringRef topFuncName) {
    auto builder = OpBuilder(mod);
    FuncOp topFunc;
    if(!topFind(mod, topFunc, topFuncName)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }

    SmallVector<Operation*> GraphOps;
    SmallVector<IOPopOp> IOPopOps;
    SmallVector<Value> ArgIns;
    SmallVector<Value> ArgOuts;
    OpCollect(topFunc, GraphOps, IOPopOps, ArgIns, ArgOuts);

    //Create an empty func adf_graph before the outmost band loop
    builder.setInsertionPoint(topFunc);
    auto funcName = "adf_graph";
    auto funcType = builder.getFunctionType(ValueRange(ArgIns), ValueRange(ArgOuts));
    auto newfunc = builder.create<FuncOp>(builder.getUnknownLoc(), funcName, funcType);
    newfunc->setAttr("adf.graph",builder.getUnitAttr());
    auto destBlock = newfunc.addEntryBlock();
    builder.setInsertionPointToEnd(destBlock);
    auto returnOp = builder.create<ReturnOp>(builder.getUnknownLoc(),ValueRange(ArgOuts));

    // Create the function CallOp
    Block &entryBlock = topFunc.getBody().front();
    auto topreturnOp = dyn_cast<ReturnOp>(entryBlock.getTerminator());

    builder.setInsertionPoint(topreturnOp);
    auto callop = builder.create<CallOp>(topreturnOp.getLoc(), newfunc, ValueRange(ArgIns));

    builder.setInsertionPoint(returnOp);
    for (Operation *op : GraphOps) {
        op->moveBefore(returnOp);
    }

    builder.setInsertionPoint(topreturnOp);
    for (auto op : IOPopOps) {
        op->moveBefore(topreturnOp);
    }

    // Update the references in the newfunc after move
    for (unsigned i = 0, num_arg = destBlock->getNumArguments(); i < num_arg; ++i) {
        auto sourceArg = ArgIns[i];
        auto destArg = destBlock->getArgument(i);
        sourceArg.replaceUsesWithIf(destArg,[&](OpOperand &use){
            return newfunc->isProperAncestor(use.getOwner());
        });
    }

    // Update the returned value in the topFunc
    for (unsigned i = 0, num_res = callop.getNumResults(); i < num_res; ++i) {
        auto sourceArg = ArgOuts[i];
        auto destArg = callop.getResult(i);
        sourceArg.replaceUsesWithIf(destArg,[&](OpOperand &use){
            return topFunc->isProperAncestor(use.getOwner());
        });
    }

    return true;
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesADFGraphCreatePass() {
  return std::make_unique<AriesADFGraphCreate>();
}

} // namespace aries
} // namespace mlir