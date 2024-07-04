#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Dialect/Affine/Passes.h"
#include "llvm/Support/Debug.h"
#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "aries/Dialect/ADF/ADFDialect.h"

using namespace mlir;
using namespace aries;
using namespace adf;
using namespace mlir::memref;

namespace {

struct AriesKernelInterfaceCreate : public AriesKernelInterfaceCreateBase<AriesKernelInterfaceCreate> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";
    if (!KernelInterfaceCreate(mod,topFuncName))
      signalPassFailure();
  }

private:
  // This is an experimental pass to create the output buffer
  // TODO: Tensor may be a proper representation for the arguments of the adf kernel
  bool KernelInterfaceCreate (ModuleOp mod, StringRef topFuncName) {
    auto builder = OpBuilder(mod);
    FuncOp topFunc;
    if(!topFind(mod, topFunc, topFuncName)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }

    PassManager pm(&getContext());
    pm.addPass(createLoopFusionPass());
    auto loc = builder.getUnknownLoc();
    auto result = topFunc.walk([&](CallOp caller){
      auto calleeFuncOp = mod.lookupSymbol<FuncOp>(caller.getCallee());
      auto &entryBlock = calleeFuncOp.getBody().front();
      auto returnOpOld = dyn_cast<ReturnOp>(entryBlock.getTerminator());
      auto inTypes =calleeFuncOp.getArgumentTypes();
      auto outTypes =SmallVector<Type,8>(calleeFuncOp.getResultTypes().begin(),
                                         calleeFuncOp.getResultTypes().end());
      SmallVector<Value, 4> returnOperands(returnOpOld.getOperands().begin(), 
                                           returnOpOld.getOperands().end());
      
      // check if the callee memref type arguments have been touched or not 
      unsigned index = 0;
      SmallVector<unsigned, 4> ArgIndeices;
      for (auto arg : calleeFuncOp.getArguments()) {
        if(auto type = dyn_cast<MemRefType>(arg.getType())){
          for (auto user : arg.getUsers()){
            if (dyn_cast<AffineStoreOp>(user)){
              //If touched, then alloc and copy to a new buffer and return it
              builder.setInsertionPoint(returnOpOld);
              auto buffer = builder.create<AllocOp>(loc,MemRefType::get(type.getShape(),type.getElementType(),AffineMap(),type.getMemorySpaceAsInt()));
              builder.setInsertionPointAfter(buffer);
              
              //Create nested for loops
              SmallVector<Value, 4> ivs;
              for (int i = 0, e = type.getRank(); i < e; ++i) {
                auto ub = type.getDimSize(i);
                auto loop = builder.create<AffineForOp>(loc, 0, ub);
                ivs.push_back(loop.getInductionVar());
                builder.setInsertionPointToStart(loop.getBody());
              }

              // Load from source and store to destination
              auto value = builder.create<AffineLoadOp>(loc, arg, ivs);
              builder.create<AffineStoreOp>(loc, value, buffer, ivs);
              outTypes.push_back(type);
              returnOperands.push_back(buffer);
              ArgIndeices.push_back(index);
            }
          }
        }
        index++;
      }

      //Update the callee function type and erase the old returnOp
      auto newFuncType = FunctionType::get(calleeFuncOp.getContext(), inTypes, outTypes);
      calleeFuncOp.setType(newFuncType);
      builder.setInsertionPoint(returnOpOld);
      builder.create<ReturnOp>(loc, returnOperands);
      returnOpOld.erase();

      OpBuilder builder(caller);
      auto newCallOp = builder.create<func::CallOp>(caller.getLoc(), SymbolRefAttr::get(calleeFuncOp), outTypes, caller.getOperands());

      // Update the uses of the old call results to use the new call results
      index = 0;
      for (auto argIndex : ArgIndeices) {
        auto arg = caller.getArgOperands()[argIndex];
        //Replace the touched memref by the return value if it appears after 
        // the new caller function
        arg.replaceUsesWithIf(newCallOp.getResult(index++), [&](OpOperand &use){
          if(newCallOp->isBeforeInBlock(use.getOwner()))
            return true;
          else
            return false;
        });
      }

      // Erase the old call operation
      caller.erase();
      
      //Apply Loop funsion to the kernel func
      if(failed(pm.run(calleeFuncOp)))
        return WalkResult::interrupt(); 

      return WalkResult::advance();
    });

    if (result.wasInterrupted())
      return false;

    return true;
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesKernelInterfaceCreatePass() {
  return std::make_unique<AriesKernelInterfaceCreate>();
}

} // namespace aries
} // namespace mlir