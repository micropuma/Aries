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
    //PLIOCreate(mod);
    //GraphIOCreate(mod);
    //KernelIOCreate(mod);
    IOPushOpCreate(mod);
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

  bool GraphIOCreate(ModuleOp mod){
    auto builder = OpBuilder(mod);
    builder.setInsertionPointToStart(mod.getBody());
    auto plioOp = builder.create<CreateGraphIOOp>(builder.getUnknownLoc(),PLIOType::get(builder.getContext(), mlir::aries::adf::PortDir::In, (unsigned)mlir::aries::adf::PortWidth::Width128), mlir::aries::adf::GraphIOName::PLIO);
    auto gmioOp = builder.create<CreateGraphIOOp>(builder.getUnknownLoc(),GMIOType::get(builder.getContext(), mlir::aries::adf::PortDir::In, (unsigned)mlir::aries::adf::PortWidth::Width128), mlir::aries::adf::GraphIOName::GMIO);
    auto portOp = builder.create<CreateGraphIOOp>(builder.getUnknownLoc(),PortType::get(builder.getContext(), mlir::aries::adf::PortDir::In)                                                 , mlir::aries::adf::GraphIOName::PORT);
    return true;
  }

  bool KernelIOCreate(ModuleOp mod){
    auto builder = OpBuilder(mod);
    builder.setInsertionPointToStart(mod.getBody());
    auto streamOp = builder.create<CreateKernelIOOp>(builder.getUnknownLoc(),StreamType::get(builder.getContext()), mlir::aries::adf::KernelIOName::Stream);
    auto cascadeOp = builder.create<CreateKernelIOOp>(builder.getUnknownLoc(),CascadeType::get(builder.getContext()), mlir::aries::adf::KernelIOName::Cascade);
    return true;
  }

  bool IOPushOpCreate(ModuleOp mod){
    auto builder = OpBuilder(mod);
    builder.setInsertionPointToStart(mod.getBody());
    auto plioOp = builder.create<CreateGraphIOOp>(builder.getUnknownLoc(),PLIOType::get(builder.getContext(), mlir::aries::adf::PortDir::In, (unsigned)mlir::aries::adf::PortWidth::Width128), mlir::aries::adf::GraphIOName::PLIO);
    mlir::Type elementType = builder.getF32Type();
    mlir::MemRefType memrefType = mlir::MemRefType::get({128}, elementType);
    auto alloc = builder.create<mlir::memref::AllocOp>(builder.getUnknownLoc(), memrefType);
    SmallVector<Value> src;
    src.push_back(alloc.getResult());
    SmallVector<Value> dst;
    dst.push_back(plioOp.getResult());
    auto ioPushOp = builder.create<IOPushOp>(builder.getUnknownLoc(),src,dst);
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