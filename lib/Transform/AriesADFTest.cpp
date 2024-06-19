#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/Support/Debug.h"
#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
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
    //BufferCreate(mod);
    //IOPushOpCreate(mod);
    GraphOpCreate(mod);
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

  bool BufferCreate(ModuleOp mod){
    auto builder = OpBuilder(mod);
    builder.setInsertionPointToStart(mod.getBody());
    //Create source memref
    mlir::Type elementType = builder.getF32Type();
    mlir::MemRefType memrefType = mlir::MemRefType::get({64,64}, elementType);
    auto bufferOp = builder.create<BufferOp>(builder.getUnknownLoc(), memrefType);
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
    
    //Create source memref
    mlir::Type elementType = builder.getF32Type();
    mlir::MemRefType memrefType = mlir::MemRefType::get({64,64}, elementType);
    auto allocOp = builder.create<mlir::memref::AllocOp>(builder.getUnknownLoc(), memrefType);
    auto src = allocOp.getResult();

    //Add src_offsets, src_sizes, src_strides
    auto indexType = builder.getIndexType();
    auto zeroAttr = builder.getIntegerAttr(indexType, 0);
    auto oneAttr = builder.getIntegerAttr(indexType, 1);
    auto cons32Attr = builder.getIntegerAttr(indexType, 32);
    builder.setInsertionPointAfter(allocOp);
    auto zeroValue = builder.create<arith::ConstantOp>(builder.getUnknownLoc(), indexType, zeroAttr);
    auto oneValue = builder.create<arith::ConstantOp>(builder.getUnknownLoc(), indexType, oneAttr);
    auto cons32Value = builder.create<arith::ConstantOp>(builder.getUnknownLoc(), indexType, cons32Attr);
    
    SmallVector<Value> src_offsets;
    SmallVector<Value> src_sizes;
    SmallVector<Value> src_strides;
    src_offsets.push_back(zeroValue);
    src_offsets.push_back(zeroValue);
    src_sizes.push_back(cons32Value);
    src_sizes.push_back(cons32Value);
    src_strides.push_back(oneValue);
    src_strides.push_back(oneValue);

    // Create dst plios
    SmallVector<Value> dst;
    builder.setInsertionPointAfter(cons32Value);
    auto plioOp0 = builder.create<CreateGraphIOOp>(builder.getUnknownLoc(),PLIOType::get(builder.getContext(), mlir::aries::adf::PortDir::In, (unsigned)mlir::aries::adf::PortWidth::Width128), mlir::aries::adf::GraphIOName::PLIO);
    auto plioOp1 = builder.create<CreateGraphIOOp>(builder.getUnknownLoc(),PLIOType::get(builder.getContext(), mlir::aries::adf::PortDir::In, (unsigned)mlir::aries::adf::PortWidth::Width128), mlir::aries::adf::GraphIOName::PLIO);
    dst.push_back(plioOp0.getResult());
    dst.push_back(plioOp1.getResult());
    builder.setInsertionPointAfter(plioOp1);
    auto ioPushOp = builder.create<IOPushOp>(builder.getUnknownLoc(),src,src_offsets,src_sizes,src_strides,dst);
    return true;
  }

  bool GraphOpCreate(ModuleOp mod){
    auto builder = OpBuilder(mod);
    builder.setInsertionPointToStart(mod.getBody());
    SmallVector<Value> GraphIOs;
    auto graphOp = builder.create<GraphOp>(builder.getUnknownLoc(),GraphIOs);
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