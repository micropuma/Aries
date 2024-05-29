#include "mlir/Transforms/DialectConversion.h"
#include "aries/Conversion/LowerToADF/LowerToADF.h"
#include "aries/Dialect/ADF/ADFDialect.h"

namespace mlir {
namespace aries {
namespace adf {

#define GEN_PASS_DEF_LOWERTOADF
#include "aries/Conversion/LowerToADF/LowerToADF.h.inc"

struct LowerToADF : impl::LowerToADFBase<LowerToADF> {
  using LowerToADFBase::LowerToADFBase;

  void runOnOperation() override {
    MLIRContext *context = &getContext();
    auto *module = getOperation();

    ConversionTarget target(*context);       // <--- new thing
    target.addIllegalDialect<ADFDialect>();

    RewritePatternSet patterns(context);

    if (failed(applyPartialConversion(module, target, std::move(patterns)))) {   // <-- new thing
      signalPassFailure();
    }
  }
};

}  // namespace adf
}  // namespace aries
}  // namespace mlir