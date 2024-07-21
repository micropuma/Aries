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

struct AriesADFCellCreate : public AriesADFCellCreateBase<AriesADFCellCreate> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";
  
    if (!ADFCellCreate(mod,topFuncName))
      signalPassFailure();
  }

private:
  void OpCollect(CellOp cellop, 
                 SmallVector<Operation*> &TopOps, 
                 SmallVector<Operation*> &GraphOps, 
                 SmallVector<IOPopOp> &IOPopOps, 
                 SmallVector<Value> &ArgIns,
                 SmallVector<Value> &ArgOuts){
    cellop.walk([&](Operation *op){
      if( dyn_cast<BufferOp>(op) || dyn_cast<ConnectOp>(op) || 
          dyn_cast<CallOp>(op)   || dyn_cast<ConfigPLIOOp>(op) ||
          dyn_cast<ConfigGMIOOp>(op)){
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
          TopOps.push_back(op);
          ArgIns.push_back(graphio.getResult());
        }else if(portDir == PortDir::Out){
          GraphOps.push_back(op);
          ArgOuts.push_back(graphio.getResult());
        }
      }else if(auto iopopOp = dyn_cast<IOPopOp>(op)){
        IOPopOps.push_back(iopopOp);
      }else{
        TopOps.push_back(op);
      }
    });
  }

  bool ADFCellCreate (ModuleOp mod, StringRef topFuncName) {
    auto builder = OpBuilder(mod);
    auto loc = builder.getUnknownLoc();
    FuncOp topFunc;
    if(!topFind(mod, topFunc, topFuncName)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }

    // Find the CellOp
    // TODO: Handle Multiple CellOps
    CellOp cellOp;
    for (auto op : topFunc.getOps<CellOp>()) {
      cellOp = op;
    }

    // Eliminate the cell.end Op
    cellOp.getBody().front().back().erase();
    
    SmallVector<Operation*> TopOps;
    SmallVector<Operation*> GraphOps;
    SmallVector<IOPopOp> IOPopOps;
    SmallVector<Value> ArgIns;
    SmallVector<Value> ArgOuts;
    OpCollect(cellOp, TopOps, GraphOps, IOPopOps, ArgIns, ArgOuts);

    //Create an empty func adf_graph before the outmost band loop
    builder.setInsertionPoint(topFunc);
    auto funcName = "adf_" + cellOp.getCellName().str();
    auto funcType = builder.getFunctionType(ValueRange(ArgIns), ValueRange(ArgOuts));
    auto newfunc = builder.create<FuncOp>(loc, funcName, funcType);
    newfunc->setAttr("adf.cell",builder.getUnitAttr());
    auto destBlock = newfunc.addEntryBlock();
    builder.setInsertionPointToEnd(destBlock);
    auto returnOp = builder.create<ReturnOp>(loc,ValueRange(ArgOuts));

    //Move GraphOps into adf_cell
    builder.setInsertionPoint(returnOp);
    for (Operation *op : GraphOps) {
        op->moveBefore(returnOp);
    }

    // Create the function CallOp
    Block &entryBlock = topFunc.getBody().front();
    auto topreturnOp = dyn_cast<ReturnOp>(entryBlock.getTerminator());

    builder.setInsertionPoint(topreturnOp);
    auto cellLaunchop = builder.create<LauchCellOp>(loc);
    Block *cellLaunchBlock = builder.createBlock(&cellLaunchop.getRegion());
    builder.setInsertionPointToEnd(cellLaunchBlock);
    auto endCellOp = builder.create<EndLauchCellOp>(loc);

    builder.setInsertionPoint(endCellOp);
    for (auto op : TopOps) {
        op->moveBefore(endCellOp);
    }

    builder.setInsertionPoint(endCellOp);
    auto callop = builder.create<CallOp>(loc, newfunc, ValueRange(ArgIns));
    callop->setAttr("adf.cell",builder.getUnitAttr());

    builder.setInsertionPoint(endCellOp);
    for (auto op : IOPopOps) {
        op->moveBefore(endCellOp);
    }

    // Update the references in the newfunc after move
    for (unsigned i = 0, num_arg = destBlock->getNumArguments(); i < num_arg; ++i) {
        auto sourceArg = ArgIns[i];
        auto destArg = destBlock->getArgument(i);
        sourceArg.replaceUsesWithIf(destArg,[&](OpOperand &use){
            return newfunc->isProperAncestor(use.getOwner());
        });
    }

    // Update the returned value in the cellLaunch region
    for (unsigned i = 0, num_res = callop.getNumResults(); i < num_res; ++i) {
        auto sourceArg = ArgOuts[i];
        auto destArg = callop.getResult(i);
        sourceArg.replaceUsesWithIf(destArg,[&](OpOperand &use){
            return cellLaunchop->isProperAncestor(use.getOwner());
        });
    }

    //Erase cellop after func with adf.cell attributes has been created 
    cellOp.erase();

    //Move the constantOp to the top_func
    builder.setInsertionPointToStart(&entryBlock);
    cellLaunchop.walk([&](arith::ConstantOp op){
        op->moveBefore(&entryBlock, entryBlock.begin());
    });

    return true;
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesADFCellCreatePass() {
  return std::make_unique<AriesADFCellCreate>();
}

} // namespace aries
} // namespace mlir