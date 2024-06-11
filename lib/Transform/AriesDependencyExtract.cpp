#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "llvm/Support/Debug.h"
#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "aries/Dialect/ADF/ADFDialect.h"

using namespace mlir;
using namespace aries;
using namespace mlir::aries::adf;

namespace {

struct AriesDependencyExtract : public AriesDependencyExtractBase<AriesDependencyExtract> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
  
    if (!DependencyExtract(mod))
      signalPassFailure();
  }

private:
  bool DependencyExtract (ModuleOp mod) {
    
    return true;
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesDependencyExtractPass() {
  return std::make_unique<AriesDependencyExtract>();
}

} // namespace aries
} // namespace mlir