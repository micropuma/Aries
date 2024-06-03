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
      
      auto newOp = rewriter.replaceOpWithNewOp<BufferOp>(op, op.getType());
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
    
    target.addIllegalOp<SubViewOp>();
    target.addIllegalOp<CopyOp>();
    target.addIllegalOp<AllocOp>();
    target.addIllegalOp<DeallocOp>();
    target.addIllegalOp<CallOp>();
    patterns.add<AllocConvert>(patterns.getContext());
    target.addLegalOp<FuncOp>();
    target.addLegalOp<BufferOp>();
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