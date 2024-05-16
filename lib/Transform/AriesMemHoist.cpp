#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/Dialect/Affine/Analysis/LoopAnalysis.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "llvm/Support/Debug.h"

#include "mlir/IR/Builders.h"

using namespace mlir;
using namespace mlir::memref;
using namespace aries;
using namespace func;


namespace {

struct AriesMemHoist : public AriesMemHoistBase<AriesMemHoist> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";
  
    if (!MemHoist(mod,topFuncName))
      signalPassFailure();
  }

private:
  bool MemHoist (ModuleOp mod,StringRef topFuncName) {
    auto builder = OpBuilder(mod);
    FuncOp topFunc;
    if(!topFind(mod, topFunc, topFuncName)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }

    FuncOp calleeFuncOp;
    if(!calleeFind(mod ,topFunc, calleeFuncOp)){
      calleeFuncOp->emitOpError("Callee function not found\n");
      return false;
    }

    if(!subviewHoist(builder, topFunc, calleeFuncOp)){
      return false;
    }

    

    return true;
  }

  bool subviewHoist(OpBuilder builder, FuncOp topFunc, FuncOp calleeFuncOp){
    topFunc.walk([&](CallOp caller){
      
      //Change the offset of the subview to the caller arguments
      for (auto arg : calleeFuncOp.getArguments()) {
        auto firstUser = *arg.user_begin(); 
        if(dyn_cast<SubViewOp>(firstUser)&&!dyn_cast<MemRefType>(arg.getType())){
          arg.replaceAllUsesWith(caller.getOperand(arg.getArgNumber()));
        }
      }

      auto inTypes = SmallVector<Type, 8>(calleeFuncOp.getArgumentTypes().begin(),
                                           calleeFuncOp.getArgumentTypes().end());

      auto outTypes = calleeFuncOp.getResultTypes();

      for (auto subview : llvm::make_early_inc_range(calleeFuncOp.getOps<SubViewOp>())) {
        //check if the source of the subview is an argument of the callee
        auto arg = subview.getSource().dyn_cast<BlockArgument>();
        if (!arg)
          continue;
        
        //Record the corrsponding arguments in caller since the subview will be
        //replaced by the new callee arguments next
        auto callerMem = caller.getOperand(arg.getArgNumber());

        //Change the memref of callee to the type of corrsponding subview
        //And replace the use of subview by the arguments 
        inTypes[arg.getArgNumber()] = subview.getType();
        arg.setType(subview.getType());
        subview.replaceAllUsesWith(arg);
        
        //Move the subview before the caller function
        subview.getSourceMutable().assign(callerMem);
        subview->remove();
        builder.setInsertionPoint(caller);
        builder.insert(subview);

        //Change the arguments of the caller function
        caller.setOperand(arg.getArgNumber(), subview);
      }

      // Update the callee function type.
      calleeFuncOp.setType(builder.getFunctionType(inTypes, outTypes));

    });

    return true;
  }


};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesMemHoistPass() {
  return std::make_unique<AriesMemHoist>();
}

} // namespace aries
} // namespace mlir