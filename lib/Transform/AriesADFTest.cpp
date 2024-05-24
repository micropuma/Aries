#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/Support/Debug.h"
#include "mlir/IR/Builders.h"
#include "aries/Dialect/ADF/ADFDialect.h"

using namespace mlir;
using namespace aries;
using namespace mlir::aries::adf;

namespace {

struct AriesADFTest : public AriesADFTestBase<AriesADFTest> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
  
    if (!ADFTest(mod))
      signalPassFailure();
  }

private:
  bool ADFTest (ModuleOp mod) {
    PLIOCreate(mod);
    
    return true;
  }

  bool PLIOCreate(ModuleOp mod){
    auto builder = OpBuilder(mod);
    builder.setInsertionPointToStart(mod.getBody());
    auto plioOp = builder.create<PLIOOp>(builder.getUnknownLoc(),PLIOType::get(builder.getContext(), mlir::aries::adf::PortDir::In, (unsigned)mlir::aries::adf::PortWidth::Width128));
    auto type = plioOp.getType();
    llvm::outs() << "Type is :" << type << "\n" ;
    return true;
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesADFTestPass() {
  return std::make_unique<AriesADFTest>();
}

} // namespace aries
} // namespace mlir