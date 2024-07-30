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

  // Tranverse all the AffineApplyOps
  // if used by IOPush or IOPop then hoist before lauchcell
  void ApplyOpProcess(OpBuilder builder, FuncOp topFunc, LauchCellOp lauchcell){
    topFunc.walk([&](AffineApplyOp op){
      auto result = op.getResult();
      for(auto user: result.getUsers()){
        if(dyn_cast<IOPushOp>(user) || dyn_cast<IOPopOp>(user)){
          op->remove();
          builder.setInsertionPoint(lauchcell);
          builder.insert(op); 
        }
      }
    });
  }

  // Serialize the IOPushOp for GMIO
  void IOPushOpProcess(OpBuilder builder, FuncOp topFunc, 
                       LauchCellOp lauchcell){
    // Need to consider the insertion point of dma of PopOp and deallocOp
    // Now set after adf.cell.launch
    auto loc = builder.getUnknownLoc();
    auto &entryBlock = topFunc.getBody().front();
    auto returnOp = dyn_cast<ReturnOp>(entryBlock.getTerminator());
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
      builder.setInsertionPoint(returnOp);
      auto dealloc = builder.create<DeallocOp>(loc,newMem);
      dealloc->setAttr("gmio",builder.getUnitAttr());
      builder.setInsertionPoint(lauchcell);
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
  void IOPopOpProcess(OpBuilder builder, FuncOp topFunc, 
                       LauchCellOp lauchcell){
    auto loc = builder.getUnknownLoc();
    auto &entryBlock = topFunc.getBody().front();
    auto returnOp = dyn_cast<ReturnOp>(entryBlock.getTerminator());
    topFunc.walk([&](IOPopOp op){
      auto src = op.getSrc();
      auto dst   = op.getDst();
      auto dst_offsets = op.getDstOffsets();
      auto dst_sizes   = op.getDstSizes();
      auto dst_strides = op.getDstStrides();
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
      builder.setInsertionPointAfter(lauchcell);
      auto dmaOp = builder.create<DmaOp>(
                        loc, newMem, ValueRange(), ValueRange(), ValueRange(),
                        dst, dst_offsets, dst_sizes, dst_strides);
      dmaOp->setAttr("out",builder.getUnitAttr());
      builder.setInsertionPoint(returnOp);
      auto dealloc = builder.create<DeallocOp>(loc,newMem);
      dealloc->setAttr("gmio",builder.getUnitAttr()); 
      builder.setInsertionPoint(op);
      builder.create<IOPopOp>(loc, src, newMem, 
                              ValueRange(), ValueRange(), ValueRange());             
      op.erase();
      return WalkResult::advance();
    });
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
    
    // Materialize Push/Pop of GMIO
    auto boolval = topFunc->getAttr("gmio");
    if(dyn_cast<BoolAttr>(boolval).getValue()){
      ApplyOpProcess(builder, topFunc, lauchcell);
      IOPushOpProcess(builder, topFunc, lauchcell);
      IOPopOpProcess(builder, topFunc, lauchcell);
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