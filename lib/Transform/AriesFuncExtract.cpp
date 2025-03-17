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
    if (!FuncExtract(mod))
      signalPassFailure();
  }

private:
  // 将L3循环体提取为独立函数
  bool FuncExtract(ModuleOp mod) {
    auto builder = OpBuilder(mod);
    
    // Extract the affineParallelOp inside adf.func
    for (auto func : mod.getOps<FuncOp>()) {
      // 只处理adf.func
      if(!func->hasAttr("adf.func"))
        continue;
      // Find the affineParallelOp
      AffineParallelOp parallelOp 
                       = getFirstOpOfType<AffineParallelOp>(func.getBody());
      if(!parallelOp)
        return true;

      SmallVector<AffineForOp, 6> band;
      getNestedLoopBand(parallelOp.getRegion(), band);
      // There should be at least one point loop
      unsigned width = band.size();
      if (width < 1){
        func->emitOpError("The number of point loop is less than 1\n");
        return false;
      }

      // 如果parallelOp的层数大于1，提取最外层循环
      CallFuncCreation(builder, func, band, parallelOp);
    } 
    return true;
  }

  void CallFuncCreation(OpBuilder builder, FuncOp topFunc, 
                        SmallVector<AffineForOp, 6> band, 
                        AffineParallelOp parallelOp){
    // The Arguments in the specified block is not a live-in variable
    // 确保新函数参数包含所有外部依赖变量（如循环边界、输入数据指针等）。
    SmallVector<Value, 6> liveins(parallelOp.getBody()->getArguments());
    // todo：详细学一下mlir的liveness分析
    auto liveness = Liveness(parallelOp);
    // getLiveIn获取parallelOp中使用的所有live-in变量
    for (auto livein: liveness.getLiveIn(parallelOp.getBody()))
      // 即parallelOp不是livein的直接祖先
      // 即livein定义在parallelOp之外，过滤定义在循环内部的变量
      // 收集这种livein变量
      if (!parallelOp->isProperAncestor(livein.getParentBlock()->getParentOp()))
        liveins.push_back(livein);
    
    //reorder inputs to be correspond with the topfunc arguments
    SmallVector<Value, 6> inputs;
    for(auto arg : topFunc.getArguments()){
      auto it = llvm::find(liveins,arg);
      // 将topFunc中的参数，如果在liveins中，加入inputs
      if(it != liveins.end())
        inputs.push_back(arg);
    }
    for(auto livein : liveins){
      // 如果livein不在inputs中，加入inputs
      auto it = llvm::find(inputs, livein);
      if(it == inputs.end())
        inputs.push_back(livein);
    }

    builder.setInsertionPoint(topFunc);
    // Define the KNL function with the detected inputs as arguments
    auto symbol = topFunc.getSymName();
    auto funcName = "kernel_" + symbol.str();
    // 将所有的livein变量作为参数，返回值为空
    auto funcType = builder.getFunctionType(ValueRange(inputs), TypeRange({}));
    auto newfunc = builder.create<FuncOp>(
                                  builder.getUnknownLoc(), funcName, funcType);
    newfunc->setAttr("adf.kernel",builder.getUnitAttr());
    auto destBlock = newfunc.addEntryBlock();
    builder.setInsertionPointToEnd(destBlock);
    auto returnOp = builder.create<ReturnOp>(builder.getUnknownLoc());
    
    // Create the function CallOp
    auto outerPointLoop = band[0];
    builder.setInsertionPoint(outerPointLoop);
    builder.create<CallOp>(outerPointLoop.getLoc(), newfunc, inputs);

    // Move the entire block of outerPointLoop before the returnOp
    // 将外层循环体移动到returnOp之前
    builder.setInsertionPointToEnd(destBlock);
    outerPointLoop->moveBefore(returnOp);

    // 替换参数引用
    // Update the references in the newfunc after move
    auto argNum = destBlock->getNumArguments();
    for (unsigned i = 0; i < argNum; ++i) {
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

// 提取循环体为独立函数
std::unique_ptr<Pass> createAriesFuncExtractPass() {
  return std::make_unique<AriesFuncExtract>();
}

} // namespace aries
} // namespace mlir