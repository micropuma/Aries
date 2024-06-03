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
  bool LowerToADF(ModuleOp mod,StringRef topFuncName) {
    auto builder = OpBuilder(mod);
    FuncOp topFunc;

    if(!topFind(mod, topFunc, topFuncName)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }

    ConversionTarget target(*mod.getContext());
    RewritePatternSet patterns(mod.getContext());

    target.addLegalOp<SubViewOp>();
    target.addLegalOp<CopyOp>();
    target.addLegalOp<AllocaOp>();
    target.addLegalOp<DeallocOp>();
    target.addLegalOp<CallOp>();
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
