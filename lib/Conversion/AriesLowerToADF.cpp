#include "aries/Conversion/Passes.h"
#include "aries/Dialect/ADF/ADFDialect.h"
#include "aries/Transform/Utils.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"

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
      
      //if the CopyOp is copied to L1 mem
      if(op.getTarget().getType().dyn_cast<MemRefType>().getMemorySpaceAsInt() == (int)mlir::aries::adf::MemorySpace::L1){
        rewriter.setInsertionPoint(op);
        llvm::outs()<< "Print here\n" ;
        auto port = rewriter.create<CreateGraphIOOp>(rewriter.getUnknownLoc(),PortType::get(rewriter.getContext(), mlir::aries::adf::PortDir::In), mlir::aries::adf::GraphIOName::PORT);
        if (auto subViewOp = dyn_cast<SubViewOp>(op.getSource().getDefiningOp())){
          auto src = subViewOp.getSource();
          SmallVector<Value> dst;
          dst.push_back(port.getResult());
          SmallVector<Value> src_offsets = subViewOp.getOffsets();
          SmallVector<Value> src_sizes = subViewOp.getSizes();
          SmallVector<Value> src_strides = subViewOp.getStrides();
          rewriter.replaceOpWithNewOp<IOPushOp>(op, src, src_offsets,src_sizes,src_strides, dst);
          llvm::outs()<< "Print here1\n" ;
        }else {
          return failure();
        }
      }//else if(op.getSource().getType().dyn_cast<MemRefType>().getMemorySpaceAsInt() == (int)mlir::aries::adf::MemorySpace::L1){

      //}
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
    
    // target.addIllegalOp<SubViewOp>();
    target.addIllegalOp<AllocOp>();
    target.addIllegalOp<CopyOp>();
    // target.addIllegalOp<DeallocOp>();
    // target.addIllegalOp<CallOp>();
    patterns.add<AllocConvert>(patterns.getContext());
    patterns.add<CopyConvert>(patterns.getContext());
    // target.addLegalOp<FuncOp>();
    target.addLegalOp<BufferOp>();
    target.addLegalOp<IOPushOp>();
    // target.addLegalDialect<ADFDialect>();

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