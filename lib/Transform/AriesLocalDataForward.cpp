#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/Passes.h"
#include "llvm/Support/Debug.h"
#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "aries/Dialect/ADF/ADFDialect.h"

using namespace mlir;
using namespace aries;
using namespace adf;
using namespace mlir::func;

struct DMAForward: public OpRewritePattern<DmaOp>{
DMAForward(MLIRContext *context)
      : OpRewritePattern<DmaOp>(context, /*benefit=*/1) {}

  LogicalResult matchAndRewrite(
    DmaOp op, PatternRewriter &rewriter) const override {

    // 获取src和dst的信息
    auto ConSrc = op.getSrc();
    SmallVector<Value> src_offsets=op.getSrcOffsets();
    SmallVector<Value> src_sizes  =op.getSrcSizes();
    SmallVector<Value> src_strides=op.getSrcStrides();
    auto ConDst = op.getDst();
    SmallVector<Value> dst_offsets=op.getDstOffsets();
    SmallVector<Value> dst_sizes  =op.getDstSizes();
    SmallVector<Value> dst_strides=op.getDstStrides();
    
    if(op->getAttr("finish")){
      rewriter.eraseOp(op);
      return success();
    }

    // 查询dma operation是否有read或是write属性
    if(auto readAttr = op->getAttr("read")){
      auto intRAttr = dyn_cast<IntegerAttr>(readAttr);
      if(!intRAttr)
        return success();
      auto RIndex = intRAttr.getInt();
      if(RIndex<=0)
        return success();
      for(auto use: ConSrc.getUsers()){
        if(auto dmaop = dyn_cast<DmaOp>(use)){
          auto writeAttr = dmaop->getAttr("write");
          SmallVector<Value> Wdst_offsets=dmaop.getDstOffsets();
          SmallVector<Value> Wdst_sizes  =dmaop.getDstSizes();
          SmallVector<Value> Wdst_strides=dmaop.getDstStrides();
          if(writeAttr
             && src_offsets == Wdst_offsets
             && src_sizes   == Wdst_sizes
             && src_strides == Wdst_strides){
            auto intWAttr = dyn_cast<IntegerAttr>(writeAttr);
            auto WIndex = intWAttr.getInt();

            // 如果write的index是read的index-1
            // 即先读后写，可以进行优化
            if(WIndex == RIndex -1){
              auto src = dmaop.getSrc();
              SmallVector<Value> Rsrc_offsets=dmaop.getSrcOffsets();
              SmallVector<Value> Rsrc_sizes  =dmaop.getSrcSizes();
              SmallVector<Value> Rsrc_strides=dmaop.getSrcStrides();
              rewriter.setInsertionPointAfter(op);

              // 即将read 后 write变成一个dma read
              rewriter.replaceOpWithNewOp<DmaOp>
              (op,src,    Rsrc_offsets, Rsrc_sizes, Rsrc_strides, 
                  ConDst, dst_offsets,  dst_sizes,  dst_strides);
              dmaop->removeAttr("write");
              dmaop->setAttr("finish",rewriter.getUnitAttr());
              return success();
            }
          }
        }
      }
    }else if(auto writeAttr = op->getAttr("write")){
      auto intWAttr = dyn_cast<IntegerAttr>(writeAttr);
      if(!intWAttr)
        return success();
      auto WIndex = intWAttr.getInt();
      for(auto use: ConDst.getUsers()){
        if(auto dmaop = dyn_cast<DmaOp>(use)){
          auto readAttr = dmaop->getAttr("read");
          SmallVector<Value> Rsrc_offsets=dmaop.getSrcOffsets();
          SmallVector<Value> Rsrc_sizes  =dmaop.getSrcSizes();
          SmallVector<Value> Rsrc_strides=dmaop.getSrcStrides();
          if(readAttr
             && dst_offsets == Rsrc_offsets
             && dst_sizes   == Rsrc_sizes  
             && dst_strides == Rsrc_strides){
            auto intRAttr = dyn_cast<IntegerAttr>(readAttr);
            auto RIndex = intRAttr.getInt();
            if(WIndex == RIndex - 1){
              auto dst = dmaop.getDst();
              SmallVector<Value> Rdst_offsets=dmaop.getDstOffsets();
              SmallVector<Value> Rdst_sizes  =dmaop.getDstSizes();
              SmallVector<Value> Rdst_strides=dmaop.getDstStrides();
              rewriter.setInsertionPointAfter(dmaop);
              rewriter.replaceOpWithNewOp<DmaOp>
              (op, ConSrc,src_offsets, src_sizes,  src_strides,
                   dst,   Rdst_offsets,Rdst_sizes, Rdst_strides);
              dmaop->removeAttr("read");

              // finish可以支持 L2或L3到L2或L3的数据传输
              // 将read 后 Write优化掉
              dmaop->setAttr("finish",rewriter.getUnitAttr());
              return success();
            }
          }
        }
      }
    }
    return success();
  }
};

namespace {

struct AriesLocalDataForward 
      : public AriesLocalDataForwardBase<AriesLocalDataForward> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    if (!LocalDataForward(mod))
      signalPassFailure();
  }

private:

  // 核心函数，遍历所有的adf.func
  bool LocalDataForward (ModuleOp mod) {
    // Tranverse all the adf.func
    for (auto func : mod.getOps<FuncOp>()) {
      if(!func->hasAttr("adf.func"))
        continue;
      auto context = func->getContext();
      PassManager pm(context);
      pm.addPass(createCSEPass());
      pm.addPass(createCanonicalizerPass());
      if (failed(pm.run(func))) {
        return false;
      }

      // 进行模式匹配pattern rewrite
      RewritePatternSet patterns(context);
      patterns.add<DMAForward>(patterns.getContext());
      (void)applyPatternsGreedily(mod, std::move(patterns));

      // 处理带有initialize属性的dma操作，后处理
      func.walk([&](DmaOp dmaOp){
        if(dmaOp->hasAttr("initialize"))
          dmaOp.erase();
      });
    }

    return true;
  }

};
} // namespace



namespace mlir {
namespace aries {

// 优化如下case：
// 原始操作
// DmaWrite A → B
// DmaRead B → C
// 优化后
// DmaWrite A → C  // 直接写入最终目标
// 其核心功能是优化DMA（直接内存访问）操作的数据移动过程。
// 核心功能：本地缓冲区数据直传
std::unique_ptr<Pass> createAriesLocalDataForwardPass() {
  return std::make_unique<AriesLocalDataForward>();
}

} // namespace aries
} // namespace mlir