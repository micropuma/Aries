#include "aries/Transform/Passes.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Analysis/Liveness.h"

using namespace mlir;
using namespace mlir::affine;
using namespace aries;
using namespace func;


namespace {

struct AriesFuncExtract : public AriesFuncExtractBase<AriesFuncExtract> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";    
  
    FuncExtract(mod,topFuncName);
  }

private:
  void FuncExtract(ModuleOp mod,StringRef topFuncName) {
    auto b = OpBuilder(mod);
    FuncOp topFunc;
    for (FuncOp func : mod.getOps<FuncOp>()) {
      // Check if the attribute exists on this function.
      if (func->getAttr(topFuncName))
        topFunc = func;
    }

    std::vector<SmallVector<AffineForOp, 6>> bands;
    getTileableBands(topFunc, &bands);
    
    unsigned cnt_band = 0;
    for (auto band : bands) {
      
      // The band should be multiple of 2 after tiling
      unsigned width = band.size()/2;

      // Find the innermost block loop 
      auto innerBlockLoop = band[width];
      SmallVector<Value, 10> inputs(innerBlockLoop.getOperands());

      //Detect the all the arguments used in the innermost block loop 
      ArguDetect(innerBlockLoop, inputs);

      CallFuncCreation(b, topFunc, innerBlockLoop, cnt_band, inputs);
      
      
    }
  }

  void ArguDetect(AffineForOp innerBlockLoop,SmallVectorImpl<Value> &inputs){
    //Find all the liveness with in innerBlockLoop
    auto liveness = Liveness(innerBlockLoop);

    //Check each liveinVal in the block
    for (auto liveinVal: liveness.getLiveIn(innerBlockLoop.getBody()))

      //If the liveinVal is defined with the AffineForOp innerBlockLoop 
      if (!innerBlockLoop->isAncestor(liveinVal.getParentBlock()->getParentOp()))
        inputs.push_back(liveinVal);

  }

  void CallFuncCreation(OpBuilder builder, FuncOp topFunc, AffineForOp innerBlockLoop, unsigned cnt_band, SmallVectorImpl<Value>& inputs){
    builder.setInsertionPoint(topFunc);

    // Define the KNL function with the detected inputs as arguments
    auto funcName = "KNL" + std::to_string(cnt_band++);
    auto funcType = builder.getFunctionType(ValueRange(inputs), TypeRange({}));
    auto newfunc = builder.create<FuncOp>(builder.getUnknownLoc(), funcName, funcType);
    auto destBlock = newfunc.addEntryBlock();
    builder.setInsertionPointToEnd(destBlock);
    auto returnOp = builder.create<ReturnOp>(builder.getUnknownLoc());
    
    // Create the function CallOp
    builder.setInsertionPoint(innerBlockLoop);
    builder.create<CallOp>(innerBlockLoop.getLoc(), newfunc, inputs);

    // Move the entire block of innerBlockLoop before the returnOp
    builder.setInsertionPointToEnd(destBlock);
    innerBlockLoop->moveBefore(returnOp);

    // Update the references in the newfunc before move
    for (unsigned i = 0, num_arg = destBlock->getNumArguments(); i < num_arg; ++i) {
        auto sourceArg = inputs[i];
        auto destArg = destBlock->getArgument(i);
        sourceArg.replaceUsesWithIf(destArg,[&](OpOperand &use){
            return newfunc->isProperAncestor(use.getOwner());
        });
    }
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesFuncExtractPass() {
  return std::make_unique<AriesFuncExtract>();
}

} // namespace aries
} // namespace mlir