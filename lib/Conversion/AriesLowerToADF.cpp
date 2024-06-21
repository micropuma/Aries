#include "aries/Conversion/Passes.h"
#include "aries/Dialect/ADF/ADFDialect.h"
#include "aries/Transform/Utils.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/Arith/IR/Arith.h"

using namespace mlir;
using namespace aries;
using namespace adf;
using namespace mlir::memref;
using namespace mlir::func;

struct AllocConvert : public OpConversionPattern<AllocOp> {
  using OpConversionPattern<AllocOp>::OpConversionPattern;
  LogicalResult matchAndRewrite(
      AllocOp op, OpAdaptor adaptor,
      ConversionPatternRewriter &rewriter) const override {
      
      rewriter.replaceOpWithNewOp<BufferOp>(op, op.getType());
    return success();
  }
};

struct DeallocConvert : public OpConversionPattern<DeallocOp> {
  using OpConversionPattern<DeallocOp>::OpConversionPattern;
  LogicalResult matchAndRewrite(
      DeallocOp op, OpAdaptor adaptor,
      ConversionPatternRewriter &rewriter) const override {
      
      rewriter.eraseOp(op);
    return success();
  }
};

struct SubViewConvert : public OpConversionPattern<SubViewOp> {
  using OpConversionPattern<SubViewOp>::OpConversionPattern;
  LogicalResult matchAndRewrite(
      SubViewOp op, OpAdaptor adaptor,
      ConversionPatternRewriter &rewriter) const override {
      
      rewriter.eraseOp(op);
    return success();
  }
};

//Create port and replace copy to/from L1 memory with the io.push
struct CopyConvert : public OpConversionPattern<CopyOp> {
  using OpConversionPattern<CopyOp>::OpConversionPattern;
  LogicalResult matchAndRewrite(
      CopyOp op, OpAdaptor adaptor,
      ConversionPatternRewriter &rewriter) const override {
    auto CopySrc = op.getSource();
    auto CopyDst = op.getTarget();

    Value src;
    SmallVector<Value> src_offsets;
    SmallVector<Value> src_sizes;
    SmallVector<Value> src_strides;
    Value dst;
    SmallVector<Value> dst_offsets;
    SmallVector<Value> dst_sizes;
    SmallVector<Value> dst_strides;

    //Check if the src memref is defined by subviewOp
    if(auto defineOp = CopySrc.getDefiningOp()){
      if (auto subViewOp = dyn_cast<SubViewOp>(defineOp)){
        src = subViewOp.getSource();
        auto indexType = rewriter.getIndexType();
        auto mixedOffsets = subViewOp.getMixedOffsets();
        for (auto offset : mixedOffsets) {
          if (auto attr = dyn_cast<Attribute>(offset)) {//static offset
            if (auto integerAttr = dyn_cast<IntegerAttr>(attr)) {
              auto offsetAttr = rewriter.getIntegerAttr(indexType, integerAttr.getInt());
              auto offsetValue = rewriter.create<arith::ConstantOp>(op->getLoc(), indexType, offsetAttr);
              src_offsets.push_back(offsetValue);
            }
          }else if (auto value = dyn_cast<Value>(offset)) {//dynamic offset
            src_offsets.push_back(value);
          }
        }
        for(auto size : subViewOp.getStaticSizes()){
          auto sizeAttr = rewriter.getIntegerAttr(indexType, size);
          auto sizeValue = rewriter.create<arith::ConstantOp>(op->getLoc(), indexType, sizeAttr);
          src_sizes.push_back(sizeValue);
        }
        for(auto stride : subViewOp.getStaticStrides()){
          auto strideAttr = rewriter.getIntegerAttr(indexType, stride);
          auto strideValue = rewriter.create<arith::ConstantOp>(op->getLoc(), indexType, strideAttr);
          src_strides.push_back(strideValue);
        }
      }
      else{
        src = CopySrc;
      }
    }

    if(auto defineOp = CopyDst.getDefiningOp()){
      if (auto subViewOp = dyn_cast<SubViewOp>(defineOp)){
        dst = subViewOp.getSource();
        auto indexType = rewriter.getIndexType();

        auto mixedOffsets = subViewOp.getMixedOffsets();
        for (auto offset : mixedOffsets) {
          if (auto attr = dyn_cast<Attribute>(offset)) {//static offset
            if (auto integerAttr = dyn_cast<IntegerAttr>(attr)) {
              auto offsetAttr = rewriter.getIntegerAttr(indexType, integerAttr.getInt());
              auto offsetValue = rewriter.create<arith::ConstantOp>(op->getLoc(), indexType, offsetAttr);
              dst_offsets.push_back(offsetValue);
            }
          }else if (auto value = dyn_cast<Value>(offset)) {//dynamic offset
            dst_offsets.push_back(value);
          }
        }

        for(auto size : subViewOp.getStaticSizes()){
          auto sizeAttr = rewriter.getIntegerAttr(indexType, size);
          auto sizeValue = rewriter.create<arith::ConstantOp>(op->getLoc(), indexType, sizeAttr);
          dst_sizes.push_back(sizeValue);
        }

        for(auto stride : subViewOp.getStaticStrides()){
          auto strideAttr = rewriter.getIntegerAttr(indexType, stride);
          auto strideValue = rewriter.create<arith::ConstantOp>(op->getLoc(), indexType, strideAttr);
          dst_strides.push_back(strideValue);
        }
      }
      else{
        dst = CopyDst;
      }
    }

    rewriter.replaceOpWithNewOp<DmaOp>(op,src,src_offsets,src_sizes,src_strides,dst,dst_offsets,dst_sizes,dst_strides);
    return success();
  }
};



namespace {

struct AriesLowerToADF : public AriesLowerToADFBase<AriesLowerToADF> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";    
  
    if (!LowerToADF(mod,topFuncName))
      signalPassFailure();
  }

private:
  

  bool LowerToADF(ModuleOp mod,StringRef topFuncName) {
    MLIRContext &context = getContext();
    RewritePatternSet patterns(&context);

    FuncOp topFunc;
    if(!topFind(mod, topFunc, topFuncName)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }
    ConversionTarget target(context);
    
    target.addIllegalOp<AllocOp>();
    target.addIllegalOp<DeallocOp>();
    target.addIllegalOp<CopyOp>();
    target.addIllegalOp<SubViewOp>();
    patterns.add<AllocConvert>(patterns.getContext());
    patterns.add<DeallocConvert>(patterns.getContext());
    patterns.add<CopyConvert>(patterns.getContext());
    patterns.add<SubViewConvert>(patterns.getContext());
    target.addLegalOp<arith::ConstantOp>();
    target.addLegalOp<BufferOp>();
    target.addLegalOp<DmaOp>();
    target.addLegalDialect<ADFDialect>();

    if (failed(applyPartialConversion(mod, target, std::move(patterns)))) {
      return false;
    }

    return true;
  }

};

} // namespace


namespace mlir {
namespace aries {
namespace adf {

std::unique_ptr<Pass> createAriesLowerToADFPass() {
  return std::make_unique<AriesLowerToADF>();
}

} // namespace adf
} // namespace aries
} // namespace mlir