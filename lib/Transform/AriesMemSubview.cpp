#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/Dialect/Affine/Analysis/LoopAnalysis.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "llvm/Support/Debug.h"

using namespace mlir;
using namespace aries;
using namespace mlir::affine;
using namespace mlir::arith;
using namespace mlir::memref;
using namespace func;


namespace {

struct AriesMemSubview : public AriesMemSubviewBase<AriesMemSubview> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";
  
    if (!MemSubview(mod,topFuncName))
      signalPassFailure();
  }

private:
  bool MemSubview (ModuleOp mod,StringRef topFuncName) {
    auto builder = OpBuilder(mod);
    auto topFunc = *(mod.getOps<FuncOp>().begin());
    FuncOp calleeFuncOp;
    if(!calleeFind(mod, topFunc, topFuncName, calleeFuncOp)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }

    

    return true;
  }

  bool createMemsubview(FuncOp calleeFuncOp, OpBuilder builder){
    for (auto arg : calleeFuncOp.getArguments()) {
      // Traverse the memref arguments in the callee function
      if (!arg.getType().dyn_cast<MemRefType>())
        continue;

      //Here assume there is only one access pattern for each memref argument
      auto affineOp = *arg.user_begin();
      SmallVector<Value, 4> operands;
      AffineMap map;
      if (auto loadOp = dyn_cast<AffineLoadOp>(affineOp)) {
        operands = SmallVector<Value, 4>(loadOp.getMapOperands());
        map = loadOp.getAffineMap();
      } else if (auto storeOp = dyn_cast<AffineStoreOp>(affineOp)) {
        operands = SmallVector<Value, 4>(storeOp.getMapOperands());
        map = storeOp.getAffineMap();
      }

      SmallVector<Value, 4> memOffsets;
      SmallVector<int64_t, 4> memSizes;
      SmallVector<int64_t, 4> memStrides;
      SmallVector<AffineExpr, 4> affineExprs;
      //builder.create<SubViewOp>(builder.getUnknownLoc(), arg, memOffsets, memSizes, memStrides);
      auto memDim = operands.size();
      for (auto operand: operands){
        auto applyOp = dyn_cast<AffineApplyOp>(operand.getDefiningOp());
        auto applyOperands = applyOp.getOperands();
        auto applyOperandsMap = applyOp.getAffineMap();
        auto mapResult = applyOperandsMap.getResult(0);
        llvm::outs() << mapResult << "\n";
        // for (auto applyOperand: applyOperands){
        //   auto definedOp = applyOperand.getDefiningOp()->getParentOfType<Block>().getParent();
        //   if (auto funcOp = dyn_cast<FuncOp>(definedOp)){
        //     memOffsets.push_back(applyOperand);
        //   }else if (auto forOp = dyn_cast<AffineForOp>(definedOp)){
        //     auto size = forOp->getConstantUpperBound();
        //     memSizes.push_back(size);
        //   }
        // }
      }

      return true;
    }
  }

  void indexElim(FuncOp topFunc, OpBuilder builder, SmallVectorImpl<FuncOp> &CalleeList){
    unsigned num_call = 0;
    //Walk through every CallOp in the topFunc and eliminate the index arguments
    topFunc.walk([&](CallOp callerFuncOp){
      auto calleeName = callerFuncOp.getCallee().str();
      auto newcalleeName = calleeName + "_" + std::to_string(num_call++);
      
      //Find the calleeFuncOp by walking through the module
      FuncOp calleeFuncOp; 
      for(auto funcOp : CalleeList) {
        if (funcOp.getName() == calleeName) {
          calleeFuncOp = funcOp;
          break;
        }
      }
          
      //Clone the older callee function and set the newcalleeName
      builder.setInsertionPoint(topFunc);
      auto newCalleeOp = calleeFuncOp.clone();
      builder.insert(newCalleeOp);
      newCalleeOp.setName(newcalleeName);

      //Change the SymbolRef of the Caller function
      callerFuncOp->setAttr("callee", SymbolRefAttr::get(newCalleeOp));
      llvm::outs() << "This is the " << newcalleeName << " CallOp \n";
    });
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesMemSubviewPass() {
  return std::make_unique<AriesMemSubview>();
}

} // namespace aries
} // namespace mlir