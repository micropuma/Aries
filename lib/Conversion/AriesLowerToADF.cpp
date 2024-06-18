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

      //First processes all the local buffer forwarding 
      auto writeAttr = op->getAttr("write");
      auto readAttr = op->getAttr("read");
      Value externMM = nullptr;
      if((SrcSpace || DstSpace) && (SrcSpace == (int)mlir::aries::adf::MemorySpace::L1 || DstSpace == (int)mlir::aries::adf::MemorySpace::L1)){
        //Find the external memory
        if(writeAttr || readAttr){
          if(DstSpace == (int)mlir::aries::adf::MemorySpace::L1)
            externMM = CopySrc;
          else
            externMM = CopyDst;

          //First tranverse: get the length of the chain
          unsigned length = 0;
          for(auto use: externMM.getUsers()){
            if(auto copyop = dyn_cast<CopyOp>(use)){
              if(auto attr = copyop->getAttr("read")){
                auto intAttr = dyn_cast<IntegerAttr>(attr);
                llvm::outs() << "Here 1, read index: " << intAttr.getInt() << "\n";
                if(intAttr.getInt() >= length)
                  length = intAttr.getInt();
              }
            }
          }

          // Check if this operation is the first read or last write
          bool flag = true;
          if(writeAttr){
            auto intAttr = dyn_cast<IntegerAttr>(writeAttr);
            llvm::outs() << "Here 4, write index: " << intAttr.getInt() << "\n";
            if(intAttr.getInt() == length)
              flag = false;
          }
          else if(readAttr){
            auto intAttr = dyn_cast<IntegerAttr>(readAttr);
            llvm::outs() << "Here 5, read index: " << intAttr.getInt() << "\n";
            if(intAttr.getInt() == 0)
              flag = false;
          }

          if(flag){
            //Second tranverse: rearrage the copyops
            SmallVector<CopyOp> copywriteOps(length+1);
            SmallVector<CopyOp> copyreadOps(length+1);
            for(auto use: externMM.getUsers()){
              if(auto copyop = dyn_cast<CopyOp>(use)){
                if(auto attr = copyop->getAttr("write")){
                  auto intAttr = dyn_cast<IntegerAttr>(attr);
                  if(intAttr.getInt()!=length){
                    llvm::outs() << "Here 2, write index: " << intAttr.getInt() << "\n";
                    copywriteOps[intAttr.getInt()] = copyop;
                  }
                }else if(auto attr = copyop->getAttr("read")){
                  auto intAttr = dyn_cast<IntegerAttr>(attr);
                  if(intAttr.getInt()!=0){
                    llvm::outs() << "Here 3, read index: " << intAttr.getInt() << "\n";
                    copyreadOps[intAttr.getInt()] = copyop;
                  }
                }
              }
            }

            // Connect write buffer with read buffer directly
            for (unsigned i=0; i<length; i++){
              llvm::outs() << "Here Delete\n";
              auto writeOp = copywriteOps[i];
              auto readOp = copyreadOps[i+1];
              auto src = writeOp.getSource();
              auto dst = readOp.getTarget();
              rewriter.setInsertionPoint(writeOp);
              rewriter.create<ConnectOp>(writeOp->getLoc(), src, dst);
              writeOp.erase();
              readOp.erase();
              llvm::outs() << "Here Delete1\n";
            }

            return success();
          }
        }
      }

      llvm::outs() << "Here 6\n" ;
      //Then processes the copy that doesn't have the opportunity to do
      //buffer forwarding
      
      //if the CopyOp is copied to L1 mem
      if(DstSpace && DstSpace == (int)mlir::aries::adf::MemorySpace::L1){
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
            llvm::outs()<< "Enter here 1\n";
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
          llvm::outs()<< "Enter here 2\n";
          return success();
        }
      }
      else if(SrcSpace && SrcSpace == (int)mlir::aries::adf::MemorySpace::L1){ //if the CopyOp is copied from L1 mem
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
            llvm::outs()<< "Enter here 3\n";
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
          llvm::outs()<< "Enter here 4\n";
          return success();
        }
      }
    llvm::outs()<< "Enter here 5\n";
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
    target.addIllegalOp<CopyOp>();
    patterns.add<AllocConvert>(patterns.getContext());
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