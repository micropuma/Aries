#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
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
  
    if (!FuncExtract(mod,topFuncName))
      signalPassFailure();
  }

private:
  bool FuncExtract(ModuleOp mod,StringRef topFuncName) {
    auto builder = OpBuilder(mod);
    FuncOp topFunc;

    if(!topFind(mod, topFunc, topFuncName)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }

    // Find the affineParallelOp
    // TODO: Handle Multiple affineParallelOps
    AffineParallelOp parallelOp;
    for (auto affineParallelOp : topFunc.getOps<AffineParallelOp>()) {
      parallelOp = affineParallelOp;
    }

    SmallVector<AffineForOp, 6> band;
    getLoopBands(parallelOp, band);
    
    // There should be at least one point loop
    unsigned width = band.size();

    if (width < 1){
      topFunc->emitOpError("The number of point loop is less than 1\n");
      return false;
    }

    

    // Find the outermost point loop
    auto outerPointLoop = band[0];

    // The Arguments in the specified block is not a live-in variable
    SmallVector<Value, 6> inputs(parallelOp.getBody()->getArguments());

    //Detect all the arguments used in the AffineParallelOp
    ArguDetect(parallelOp, inputs);

    CallFuncCreation(builder, topFunc, outerPointLoop, inputs);
      
    return true;
  }

  void ArguDetect(AffineParallelOp parallelOp,SmallVectorImpl<Value> &inputs){
    //Find all the liveness within parallelOp
    auto liveness = Liveness(parallelOp);

    //Check each liveinVal in the block
    for (auto liveinVal: liveness.getLiveIn(parallelOp.getBody())){
      //Check if the liveinVal is defined in the AffineParallelOp 
      if (!parallelOp->isProperAncestor(liveinVal.getParentBlock()->getParentOp())){
        inputs.push_back(liveinVal);
      }
    }

  }

  void CallFuncCreation(OpBuilder builder, FuncOp topFunc, AffineForOp outerPointLoop, SmallVectorImpl<Value>& inputs){
    builder.setInsertionPoint(topFunc);
    
    // Define the KNL function with the detected inputs as arguments
    auto symbol = topFunc.getSymName();
    auto funcName = "kernel_" + symbol.str();
    auto funcType = builder.getFunctionType(ValueRange(inputs), TypeRange({}));
    auto newfunc = builder.create<FuncOp>(builder.getUnknownLoc(), funcName, funcType);
    newfunc->setAttr("adf.kernel",builder.getUnitAttr());
    auto destBlock = newfunc.addEntryBlock();
    builder.setInsertionPointToEnd(destBlock);
    auto returnOp = builder.create<ReturnOp>(builder.getUnknownLoc());
    
    // Create the function CallOp
    builder.setInsertionPoint(outerPointLoop);
    builder.create<CallOp>(outerPointLoop.getLoc(), newfunc, inputs);

    // Move the entire block of outerPointLoop before the returnOp
    builder.setInsertionPointToEnd(destBlock);
    outerPointLoop->moveBefore(returnOp);

    // Update the references in the newfunc after move
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