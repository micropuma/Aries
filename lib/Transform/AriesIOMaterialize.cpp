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
  

  bool IOMaterialize (ModuleOp mod, StringRef topFuncName) {
    auto builder = OpBuilder(mod);
    auto loc = builder.getUnknownLoc();
    FuncOp topFunc;
    if(!topFind(mod, topFunc, topFuncName)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }
    Block &entryBlock = topFunc.getBody().front();
    auto topreturnOp = dyn_cast<ReturnOp>(entryBlock.getTerminator());

    Value src;
    SmallVector<Value> src_offsets;
    SmallVector<Value> src_sizes;
    SmallVector<Value> src_strides;
    Value dst;
    SmallVector<Value> dst_offsets;
    SmallVector<Value> dst_sizes;
    SmallVector<Value> dst_strides;

    builder.setInsertionPointToStart(&entryBlock);
    // Materialize Push/Pop of GMIO 
    bool flag = true;
    DeallocOp first_dealloc;
    topFunc.walk([&](IOPushOp op){
      src         = op.getSrc();
      dst         = op.getDst();
      src_offsets = op.getSrcOffsets();
      src_sizes   = op.getSrcSizes();
      src_strides = op.getSrcStrides();
      SmallVector<int64_t, 4> sizes;
      for (auto size : src_sizes)
        sizes.push_back(dyn_cast<IntegerAttr>(size.getDefiningOp<arith::ConstantOp>().getValue()).getInt());
      auto memRefType = MemRefType::get(sizes,dyn_cast<MemRefType>(src.getType()).getElementType());
      auto newMem = builder.create<AllocOp>(loc,memRefType);
      newMem->setAttr("gmio",builder.getUnitAttr());
      builder.setInsertionPoint(topreturnOp);
      auto deallocOp = builder.create<DeallocOp>(loc,newMem);
      if(flag){
        first_dealloc = deallocOp;
        flag = false;
      }
      builder.setInsertionPointAfter(newMem);
      auto dmaOp = builder.create<DmaOp>(
                            loc, src, src_offsets, src_sizes, src_strides,
                              newMem, ValueRange(), ValueRange(), ValueRange());
      dmaOp->setAttr("in",builder.getUnitAttr());
      builder.setInsertionPoint(op);
      builder.create<IOPushOp>(loc, newMem, ValueRange(), ValueRange(), ValueRange(), dst);             
      builder.setInsertionPoint(dmaOp);
      op.erase();
    });

    builder.setInsertionPointToStart(&entryBlock);
    topFunc.walk([&](IOPopOp op){
      src = op.getSrc();
      dst   = op.getDst();
      dst_offsets = op.getDstOffsets();
      dst_sizes   = op.getDstSizes();
      dst_strides = op.getDstStrides();
      SmallVector<int64_t, 4> sizes;
      for (auto size : dst_sizes)
        sizes.push_back(dyn_cast<IntegerAttr>(size.getDefiningOp<arith::ConstantOp>().getValue()).getInt());
      auto memRefType = MemRefType::get(sizes,dyn_cast<MemRefType>(dst.getType()).getElementType());
      auto newMem = builder.create<AllocOp>(loc,memRefType);
      newMem->setAttr("gmio",builder.getUnitAttr());
      builder.setInsertionPoint(topreturnOp);
      builder.create<DeallocOp>(loc,newMem);
      builder.setInsertionPoint(first_dealloc);
      auto dmaOp = builder.create<DmaOp>(
                          loc, newMem, ValueRange(), ValueRange(), ValueRange(),
                          dst, dst_offsets, dst_sizes, dst_strides);
      dmaOp->setAttr("out",builder.getUnitAttr());                    
      builder.setInsertionPoint(op);
      builder.create<IOPopOp>(loc, src, newMem, ValueRange(), ValueRange(), ValueRange());             
      builder.setInsertionPointToStart(&entryBlock);
      op.erase();
    });

    builder.setInsertionPointToStart(&entryBlock);
    topFunc.walk([&](arith::ConstantOp op){
        op->moveBefore(&entryBlock, entryBlock.begin());
    });
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