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
  // struct AllocaConvert : public OpConversionPattern<AllocaOp> {
  //   AllocaConvert(mlir::MLIRContext *context)
  //       : OpConversionPattern<AllocaOp>(context) {}

  //   using OpConversionPattern::OpConversionPattern;

  //   LogicalResult matchAndRewrite(
  //       AllocaOp op, OpAdaptor adaptor,
  //       ConversionPatternRewriter &rewriter) const override {
      
      
  //       rewriter.replaceOpWithNewOp<BufferOp>(op, op.getType());

  //     return success();
  //   }
  // };

  bool LowerToADF(ModuleOp mod,StringRef topFuncName) {
    auto builder = OpBuilder(mod);
    FuncOp topFunc;

    if(!topFind(mod, topFunc, topFuncName)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }

    // ConversionTarget target(*mod.getContext());
    // RewritePatternSet patterns(mod.getContext());

    // // target.addIllegalOp<SubViewOp>();
    // // target.addIllegalOp<CopyOp>();
    // target.addIllegalOp<AllocaOp>();
    // // target.addIllegalOp<DeallocOp>();
    // // target.addIllegalOp<CallOp>();

    // llvm::outs() << "Print this\n";
    // patterns.add<AllocaConvert>(patterns.getContext());
    
    // //target.addLegalOp<FuncOp>();
    // target.addLegalOp<BufferOp>();
    // target.addLegalDialect<ADFDialect>();

    // if (failed(applyPartialConversion(mod, target, std::move(patterns)))) {
    //   signalPassFailure();
    // }


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
