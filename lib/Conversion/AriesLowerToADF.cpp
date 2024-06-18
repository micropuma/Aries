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

//Create port and replace copy to/from L1 memory with the io.push
struct CopyConvert : public OpConversionPattern<CopyOp> {
  using OpConversionPattern<CopyOp>::OpConversionPattern;
  LogicalResult matchAndRewrite(
      CopyOp op, OpAdaptor adaptor,
      ConversionPatternRewriter &rewriter) const override {
    auto CopySrc = op.getSource();
    auto CopyDst = op.getTarget();
    auto SrcSpace = CopySrc.getType().dyn_cast<MemRefType>().getMemorySpaceAsInt();
    auto DstSpace = CopyDst.getType().dyn_cast<MemRefType>().getMemorySpaceAsInt();
    
    if(op->getAttr("finish")){
      rewriter.eraseOp(op);
      return success();
    }

    //if the CopyOp is copied to L1 mem
    if(DstSpace && DstSpace == (int)mlir::aries::adf::MemorySpace::L1){
      if(auto readAttr = op->getAttr("read")){
        auto intRAttr = dyn_cast<IntegerAttr>(readAttr);
        auto RIndex = intRAttr.getInt();
        for(auto use: CopySrc.getUsers()){
          if(auto copyop = dyn_cast<CopyOp>(use)){
            if(auto writeAttr = copyop->getAttr("write")){
              auto intWAttr = dyn_cast<IntegerAttr>(writeAttr);
              auto WIndex = intWAttr.getInt();
              if(WIndex == RIndex -1){
                auto src = copyop.getSource();
                auto dst = op.getTarget();
                rewriter.setInsertionPointAfter(op);
                rewriter.replaceOpWithNewOp<ConnectOp>(op, src,dst);
                copyop->removeAttr("write");
                copyop->setAttr("finish",rewriter.getUnitAttr());
                return success();
              }
            }
          }
        }
      }

      //Check if the src memref is defined by subviewOp
      if(auto defineOp = CopySrc.getDefiningOp()){
        if (auto subViewOp = dyn_cast<SubViewOp>(defineOp)){
          rewriter.setInsertionPoint(op);
          auto port = rewriter.create<CreateGraphIOOp>(op->getLoc(),PortType::get(rewriter.getContext(), mlir::aries::adf::PortDir::In), mlir::aries::adf::GraphIOName::PORT);
          auto src = subViewOp.getSource();
          SmallVector<Value> dst;
          dst.push_back(port.getResult());
          SmallVector<Value> src_offsets;
          SmallVector<Value> src_sizes;
          SmallVector<Value> src_strides;
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

          auto newOp = rewriter.replaceOpWithNewOp<ConnectOp>(op, port,CopyDst);
          rewriter.setInsertionPoint(newOp);
          rewriter.create<IOPushOp>(newOp->getLoc(),src, src_offsets,src_sizes,src_strides, dst);
          return success();
        }
      }else{
        rewriter.setInsertionPoint(op);
        auto port = rewriter.create<CreateGraphIOOp>(op->getLoc(),PortType::get(rewriter.getContext(), mlir::aries::adf::PortDir::In), mlir::aries::adf::GraphIOName::PORT);
        
        SmallVector<Value> dst;
        dst.push_back(port.getResult());
        
        SmallVector<Value> src_offsets;
        SmallVector<Value> src_sizes;
        SmallVector<Value> src_strides;
        
        auto newOp = rewriter.replaceOpWithNewOp<ConnectOp>(op, port,CopyDst);
        rewriter.setInsertionPoint(newOp);
        rewriter.create<IOPushOp>(newOp->getLoc(), CopySrc, src_offsets,src_sizes,src_strides, dst);
        return success();
      }
    }
    else if(SrcSpace && SrcSpace == (int)mlir::aries::adf::MemorySpace::L1){ //if the CopyOp is copied from L1 mem
      if(auto writeAttr = op->getAttr("write")){
        auto intWAttr = dyn_cast<IntegerAttr>(writeAttr);
        auto WIndex = intWAttr.getInt();
        for(auto use: CopyDst.getUsers()){
          if(auto copyop = dyn_cast<CopyOp>(use)){
            if(auto readAttr = copyop->getAttr("read")){
              auto intRAttr = dyn_cast<IntegerAttr>(readAttr);
              auto RIndex = intRAttr.getInt();
              if(WIndex == RIndex - 1){
                auto src = op.getSource();
                auto dst = copyop.getTarget();
                rewriter.setInsertionPointAfter(copyop);
                rewriter.replaceOpWithNewOp<ConnectOp>(op, src,dst);
                copyop->removeAttr("read");
                copyop->setAttr("finish",rewriter.getUnitAttr());
                return success();
              }
            }
          }
        }
      }
      
      if(auto defineOp = CopyDst.getDefiningOp()){
        if (auto subViewOp = dyn_cast<SubViewOp>(defineOp)){
          rewriter.setInsertionPoint(op);
          auto port = rewriter.create<CreateGraphIOOp>(op->getLoc(),PortType::get(rewriter.getContext(), mlir::aries::adf::PortDir::Out), mlir::aries::adf::GraphIOName::PORT);
          auto dst = subViewOp.getSource();
          SmallVector<Value> dst_offsets;
          SmallVector<Value> dst_sizes;
          SmallVector<Value> dst_strides;
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

          auto newOp = rewriter.replaceOpWithNewOp<ConnectOp>(op, CopySrc, port);
          rewriter.setInsertionPointAfter(newOp);
          rewriter.create<IOPopOp>(newOp->getLoc(),port, dst, dst_offsets,dst_sizes,dst_strides);
          return success();
        }
      }else{
        rewriter.setInsertionPoint(op);
        auto port = rewriter.create<CreateGraphIOOp>(op->getLoc(),PortType::get(rewriter.getContext(), mlir::aries::adf::PortDir::Out), mlir::aries::adf::GraphIOName::PORT);
        
        SmallVector<Value> dst_offsets;
        SmallVector<Value> dst_sizes;
        SmallVector<Value> dst_strides;
        
        auto newOp = rewriter.replaceOpWithNewOp<ConnectOp>(op, CopySrc, port);
        rewriter.setInsertionPointAfter(newOp);
        rewriter.create<IOPopOp>(newOp->getLoc(), port, CopyDst, dst_offsets,dst_sizes,dst_strides);
        return success();
      }
    }
    return failure();
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
    patterns.add<AllocConvert>(patterns.getContext());
    patterns.add<DeallocConvert>(patterns.getContext());
    patterns.add<CopyConvert>(patterns.getContext());
    target.addLegalOp<arith::ConstantOp>();
    target.addLegalOp<BufferOp>();
    target.addLegalOp<CreateGraphIOOp>();
    target.addLegalOp<IOPushOp>();
    target.addLegalOp<IOPopOp>();
    target.addLegalOp<ConnectOp>();
    target.addLegalDialect<ADFDialect>();

    if (failed(applyPartialConversion(mod, target, std::move(patterns)))) {
      signalPassFailure();
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