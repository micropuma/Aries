#include "aries/Transform/Passes.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"

using namespace mlir;
using namespace aries;
using namespace mlir::affine;
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
    for (FuncOp func : mod.getOps<FuncOp>()) {
      // Check if the attribute exists in this function.
      if (func->getAttr(topFuncName)){
        topFunc = func;
        topFunc_flag = true;
      }
    }

    if(!topFunc_flag){
      topFunc->emitOpError("Top function not found\n");
      return topFunc_flag;
    }

    index_elim(topFunc,builder);

    return topFunc_flag;
  }

  void index_elim(FuncOp topFunc, OpBuilder builder){
    unsigned num_call = 0;
    //Walk through every CallOp in the topFunc and eliminate the index arguments
    topFunc.walk([&](CallOp callerFuncOp){
      auto callee_name = callerFuncOp.getCallee().str();
      auto newCalleeName = callee_name + "_" + std::to_string(num_call++);
      auto module = callerFuncOp->getParentOfType<ModuleOp>();
      
      //Find the calleeFuncOp by walking through the module
      FuncOp calleeFuncOp; 
      module.walk([&](FuncOp funcOp) {
          if (funcOp.getName() == callee_name) {
              calleeFuncOp = funcOp;
          }
      });

      //Clone the older callee function and set the newCalleeName
      builder.setInsertionPoint(topFunc);
      auto newCalleeOp = calleeFuncOp.clone();
      builder.insert(newCalleeOp);
      newCalleeOp.setName(newCalleeName);

      //Change the SymbolRef of the Caller function
      callerFuncOp->setAttr("callee", SymbolRefAttr::get(newCalleeOp));
      

      llvm::outs() << "This is the " << newCalleeName << " CallOp \n";
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