#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "llvm/Support/Debug.h"
#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"

using namespace mlir;
using namespace aries;
using namespace mlir::func;

namespace {

struct AriesADFGraphCreate : public AriesADFGraphCreateBase<AriesADFGraphCreate> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";
  
    if (!ADFGraphCreate(mod,topFuncName))
      signalPassFailure();
  }

private:

  bool ADFGraphCreate (ModuleOp mod, StringRef topFuncName) {
    auto builder = OpBuilder(mod);
    FuncOp topFunc;
    if(!topFind(mod, topFunc, topFuncName)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }

    SmallVector<AffineForOp, 6> bands;
    getLoopBands(topFunc, bands, true);

    //Create an empty adf.graph before the outmost band loop
    auto outermostband = bands[bands.size()-1];

    //Create an empty func adf_graph before the outmost band loop
    builder.setInsertionPoint(topFunc);
    auto funcName = "adf_graph";
    auto funcType = builder.getFunctionType(TypeRange({}), TypeRange({}));
    auto newfunc = builder.create<FuncOp>(builder.getUnknownLoc(), funcName, funcType);
    auto destBlock = newfunc.addEntryBlock();
    builder.setInsertionPointToEnd(destBlock);
    auto returnOp = builder.create<ReturnOp>(builder.getUnknownLoc());

    // Create the function CallOp
    builder.setInsertionPoint(outermostband);
    builder.create<CallOp>(outermostband.getLoc(), newfunc, ValueRange({}));

    // Move the entire block of outermostband before the returnOp
    builder.setInsertionPointToEnd(destBlock);
    outermostband->moveBefore(returnOp);

    return true;
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesADFGraphCreatePass() {
  return std::make_unique<AriesADFGraphCreate>();
}

} // namespace aries
} // namespace mlir