#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/Dialect/Affine/Analysis/LoopAnalysis.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "llvm/ADT/BitVector.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/Support/Debug.h"

using namespace mlir;
using namespace aries;
using namespace mlir::affine;
using namespace mlir::arith;
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
    bool topFunc_flag = false;
    SmallVector<FuncOp, 2> CalleeList;
    
    //Find topFunc and collect the Callee functions
    for (FuncOp func : mod.getOps<FuncOp>()) {
      // Check if the attribute exists in this function.
      if (func->getAttr(topFuncName)){
        topFunc = func;
        topFunc_flag = true;
      }
      else{
        CalleeList.push_back(func);
      }
    }
    if(!topFunc_flag){
      topFunc->emitOpError("Top function not found\n");
      return topFunc_flag;
    }

    // indexElim(topFunc,builder,CalleeList);
    return topFunc_flag;
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