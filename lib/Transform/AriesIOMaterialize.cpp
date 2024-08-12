#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Analysis/Liveness.h"
#include "llvm/Support/Debug.h"
#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "aries/Dialect/ADF/ADFDialect.h"

using namespace mlir;
using namespace aries;
using namespace adf;
using namespace mlir::memref;
using namespace mlir::func;
using namespace mlir::affine;

namespace {

struct AriesIOMaterialize : public AriesIOMaterializeBase<AriesIOMaterialize> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";
  
    if (!IOMaterialize(mod,topFuncName))
      signalPassFailure();
  }

private:
  // Serialize the IOPushOp for GMIO
  void GraphIOProcess(FuncOp topFunc){
    auto &entryBlock = topFunc.getBody().front();
    topFunc.walk([&](CreateGraphIOOp op){
      op->moveBefore(&entryBlock, entryBlock.begin());
    });
  }

  // Serialize the IOPushOp for GMIO
  void GMIOPushOpProcess(OpBuilder builder, FuncOp topFunc, 
                       EndLauchCellOp endlauchCell){
    // Need to consider the insertion point of dma of PopOp and deallocOp
    // Now set before EndLauchCellOp
    auto loc = builder.getUnknownLoc();
    auto &entryBlock = topFunc.getBody().front();
    topFunc.walk([&](IOPushOp op){
      auto src         = op.getSrc();
      auto dst         = op.getDst();
      auto src_offsets = op.getSrcOffsets();
      auto src_sizes   = op.getSrcSizes();
      auto src_strides = op.getSrcStrides();
      // Skip if the src of the IOPush is serialized
      if((!src_offsets.size())&&(!src_sizes.size())&&(!src_strides.size()))
        return WalkResult::advance();
      SmallVector<int64_t, 4> sizes;
      for (auto size : src_sizes){
        auto sizeAttr = dyn_cast<IntegerAttr>(
                        size.getDefiningOp<arith::ConstantOp>().getValue());
        auto sizeInt = sizeAttr.getInt();
        sizes.push_back(sizeInt);
      }
      auto memRefType 
          = MemRefType::get(sizes, 
                        dyn_cast<MemRefType>(src.getType()).getElementType());
      builder.setInsertionPointToStart(&entryBlock);
      auto newMem = builder.create<AllocOp>(loc,memRefType);
      newMem->setAttr("gmio",builder.getUnitAttr());
      builder.setInsertionPoint(endlauchCell);
      auto dealloc = builder.create<DeallocOp>(loc,newMem);
      dealloc->setAttr("gmio",builder.getUnitAttr());
      builder.setInsertionPoint(op);
      auto dmaOp = builder.create<DmaOp>(
                            loc, src, src_offsets, src_sizes, src_strides,
                            newMem, ValueRange(), ValueRange(), ValueRange());
      dmaOp->setAttr("in",builder.getUnitAttr());
      builder.setInsertionPoint(op);
      builder.create<IOPushOp>(loc, newMem, 
                               ValueRange(), ValueRange(), ValueRange(), dst);
      op.erase();
      return WalkResult::advance();
    });
  }

  // Serialize the IOPopOp for GMIO
  void GMIOPopOpProcess(OpBuilder builder, FuncOp topFunc, 
                       EndLauchCellOp endlauchCell){
    auto loc = builder.getUnknownLoc();
    auto &entryBlock = topFunc.getBody().front();
    topFunc.walk([&](IOPopOp op){
      auto src = op.getSrc();
      auto dst   = op.getDst();
      auto dst_offsets = op.getDstOffsets();
      auto dst_sizes   = op.getDstSizes();
      auto dst_strides = op.getDstStrides();

      IOWaitOp iowaitOp;
      // Get the corresponding wait operation
      for (auto user: src.getUsers()){
        if(auto iowait = dyn_cast<IOWaitOp>(user))
          iowaitOp = iowait;
      }
      // Skip if the dst of the IOPop is serialized
      if((!dst_offsets.size())&&(!dst_sizes.size())&&(!dst_strides.size()))
        return WalkResult::advance();
      SmallVector<int64_t, 4> sizes;
      for (auto size : dst_sizes){
        auto sizeAttr = dyn_cast<IntegerAttr>(
                        size.getDefiningOp<arith::ConstantOp>().getValue());
        auto sizeInt = sizeAttr.getInt();
        sizes.push_back(sizeInt);
      }
      auto memRefType 
          = MemRefType::get(sizes, 
                        dyn_cast<MemRefType>(dst.getType()).getElementType());
      builder.setInsertionPointToStart(&entryBlock);               
      auto newMem = builder.create<AllocOp>(loc,memRefType);
      newMem->setAttr("gmio",builder.getUnitAttr());
      builder.setInsertionPointAfter(iowaitOp);
      auto dmaOp = builder.create<DmaOp>(
                        loc, newMem, ValueRange(), ValueRange(), ValueRange(),
                        dst, dst_offsets, dst_sizes, dst_strides);
      dmaOp->setAttr("out",builder.getUnitAttr());
      builder.setInsertionPoint(endlauchCell);
      auto dealloc = builder.create<DeallocOp>(loc,newMem);
      dealloc->setAttr("gmio",builder.getUnitAttr()); 
      builder.setInsertionPoint(op);
      builder.create<IOPopOp>(loc, src, newMem, 
                              ValueRange(), ValueRange(), ValueRange());             
      op.erase();
      return WalkResult::advance();
    });
  }
  
  //Collect and remove the adf.cells and erase adf.io.wait
  void Preprocess(LauchCellOp lauchcell, SmallVectorImpl<CallOp>& calls){
    lauchcell.walk([&](Operation *op){
      if(auto call = dyn_cast<CallOp>(op)){
        if(call->hasAttr("adf.cell"))
          calls.push_back(call);
        call->remove();
      }else if(dyn_cast<IOWaitOp>(op) || dyn_cast<WaitLauchCellOp>(op)){
        op->erase();
      }
    });
  }

  void ArguDetect(LauchCellOp lauchcell, SmallVectorImpl<Value> &inputs){
    //Find all the liveness within LauchCellOp
    auto liveness = Liveness(lauchcell);
    //Check each liveinVal in the block
    for (auto liveinVal: liveness.getLiveIn(&lauchcell.getBody().front())){
      //Check if the liveinVal is defined in the AffineParallelOp 
      if (!lauchcell->isProperAncestor(
                            liveinVal.getParentBlock()->getParentOp())){
        inputs.push_back(liveinVal);
      }
    }
  }

  // Create PL func.func and pl.launch. Create Callop inside pl.launch
  void PLFuncCreation(OpBuilder builder, FuncOp topFunc, 
                      LauchCellOp lauchcell, SmallVectorImpl<Value>& inputs){
    auto loc = builder.getUnknownLoc();
    // Define the dma function with the detected inputs as arguments
    builder.setInsertionPoint(topFunc);
    auto funcName = "dma_pl";
    auto funcType = builder.getFunctionType(ValueRange(inputs), TypeRange({}));
    auto newfunc = builder.create<FuncOp>(
                                  builder.getUnknownLoc(), funcName, funcType);
    newfunc->setAttr("adf.pl",builder.getUnitAttr());
    auto destBlock = newfunc.addEntryBlock();
    builder.setInsertionPointToEnd(destBlock);
    auto returnOp = builder.create<ReturnOp>(builder.getUnknownLoc());
    
    // Move the operations in the adf.cell.lauch before the returnOp
    builder.setInsertionPointToEnd(destBlock);
    auto &entryBlock = lauchcell.getBody().front();
    for(auto &op: llvm::make_early_inc_range(entryBlock)){
      if(!dyn_cast<EndLauchCellOp>(op))
        op.moveBefore(returnOp);
    }

    // Create LauchPLOp and insert the CallOp
    builder.setInsertionPointAfter(lauchcell);
    auto launchPLOp = builder.create<LauchPLOp>(loc,funcName);
    auto *cellLaunchPLBlock = builder.createBlock(&launchPLOp.getBody());
    builder.setInsertionPointToEnd(cellLaunchPLBlock);
    auto endLaunchPL = builder.create<WaitLauchPLOp>(loc);
    builder.setInsertionPoint(endLaunchPL);
    auto callop = builder.create<CallOp>(loc, newfunc, inputs);
    callop->setAttr("adf.pl",builder.getUnitAttr());

    // Update the references in the newfunc after move
    for (unsigned i = 0, num_arg = destBlock->getNumArguments(); 
         i < num_arg; ++i) {
        auto sourceArg = inputs[i];
        auto destArg = destBlock->getArgument(i);
        sourceArg.replaceUsesWithIf(destArg,[&](OpOperand &use){
            return newfunc->isProperAncestor(use.getOwner());
        });
    }
  }

  // Move the collected adf.cell to adf.cell.launch
  void UpdateCellLaunch(OpBuilder builder, LauchCellOp lauchcell, 
                        SmallVectorImpl<CallOp>& calls){
    auto &entryBlock = lauchcell.getBody().front();
    builder.setInsertionPointToStart(&entryBlock);
    for(auto call: calls)
      builder.insert(call);
  }

  void ADFPLCreate(OpBuilder builder, FuncOp topFunc, LauchCellOp lauchcell){  
    SmallVector<CallOp> calls;
    SmallVector<Value> inputs;
    Preprocess(lauchcell, calls);
    ArguDetect(lauchcell, inputs);
    PLFuncCreation(builder, topFunc, lauchcell, inputs);
    UpdateCellLaunch(builder, lauchcell, calls);
  }

  void ADFPLAlloc(OpBuilder builder, FuncOp topFunc){
    

  }

  bool IOMaterialize (ModuleOp mod, StringRef topFuncName) {
    auto builder = OpBuilder(mod);
    FuncOp topFunc;
    if(!topFind(mod, topFunc, topFuncName)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }
    
    // Find the LauchCellOp
    // TODO: Handle Multiple LauchCellOps
    LauchCellOp lauchcell;
    topFunc.walk([&](LauchCellOp op){
      lauchcell = op;
    });
    if(!lauchcell)
      return true;
    
    auto &entryBlock = lauchcell.getBody().front();
    auto endlaunchCell = dyn_cast<EndLauchCellOp>(entryBlock.getTerminator());

    GraphIOProcess(topFunc);

    // Materialize Push/Pop of GMIO
    auto boolGMIO = topFunc->getAttr("gmio");
    auto boolPLIO = topFunc->getAttr("plio");
    if(dyn_cast<BoolAttr>(boolGMIO).getValue()){
      GMIOPushOpProcess(builder, topFunc, endlaunchCell);
      GMIOPopOpProcess(builder, topFunc, endlaunchCell);
    }else if(dyn_cast<BoolAttr>(boolPLIO).getValue()){
      ADFPLCreate(builder, topFunc, lauchcell);
    }

    return true;
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesIOMaterializePass() {
  return std::make_unique<AriesIOMaterialize>();
}

} // namespace aries
} // namespace mlir