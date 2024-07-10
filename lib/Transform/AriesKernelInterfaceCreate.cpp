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
      auto inTypes = SmallVector<Type,8>(calleeFuncOp.getArgumentTypes().begin(),
                                         calleeFuncOp.getArgumentTypes().end());
      SmallVector<Type,8> inTypesNew;
      SmallVector<Value,8> inCallerNew;
      auto outTypes =SmallVector<Type,8>(calleeFuncOp.getResultTypes().begin(),
                                         calleeFuncOp.getResultTypes().end());
      SmallVector<Value, 4> returnOperands(returnOpOld.getOperands().begin(), 
                                           returnOpOld.getOperands().end());
      
      // check if the callee memref type arguments have been touched or not 
      unsigned index = 0;
      SmallVector<unsigned, 4> OutargIndeices;
      SmallVector<unsigned, 4> InargIndeices;
      SmallVector<unsigned, 4> MoveargIndeices;
      for (auto arg : calleeFuncOp.getArguments()) {
        if(auto type = dyn_cast<MemRefType>(arg.getType())){
          bool isWrite = false;
          bool isRead = false;
          for (auto user : arg.getUsers()){
            if (dyn_cast<AffineStoreOp>(user))
              isWrite=true;
            if (dyn_cast<AffineLoadOp>(user))
              isRead=true;
          }
          //If the mem has been read and written
          //then alloc and copy to a new buffer and return it
          if(isWrite && isRead){
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
            InargIndeices.push_back(index);
            OutargIndeices.push_back(index++);
          }else if(isWrite){
            //If it has only been written 
            //then move it from Argument list to return value
            builder.setInsertionPointToStart(&entryBlock);
            auto buffer = builder.create<AllocOp>(loc,MemRefType::get(type.getShape(),type.getElementType(),AffineMap(),type.getMemorySpaceAsInt()));
            outTypes.push_back(type);
            returnOperands.push_back(buffer);
            OutargIndeices.push_back(index);
            MoveargIndeices.push_back(index++);
          }else{
            InargIndeices.push_back(index++);
          }
        }
      }

      //////////Handle callee
      // Update the new inTypes for callee
      for(auto i: InargIndeices){
        inTypesNew.push_back(inTypes[i]);
        inCallerNew.push_back(caller.getOperand(i));
      }

      //Update the callee function type and erase the old returnOp
      auto newFuncType = FunctionType::get(calleeFuncOp.getContext(), inTypesNew, outTypes);
      calleeFuncOp.setType(newFuncType);

      //Update the use of the callee arguments need to be removed
      index = 0;
      for (auto i:MoveargIndeices){
        auto arg = calleeFuncOp.getArgument(i);
        arg.replaceAllUsesWith(returnOperands[index++]);
      }

      //Erase the unsed block arguments in the callee
      BitVector bv(calleeFuncOp.getArguments().size());
      for (auto i:MoveargIndeices)
        bv.set(i);
      entryBlock.eraseArguments(bv);

      //Update the return values
      builder.setInsertionPoint(returnOpOld);
      builder.create<ReturnOp>(loc, returnOperands);
      returnOpOld.erase();

      //////////Handle caller
      OpBuilder builder(caller);
      auto newCallOp = builder.create<func::CallOp>(caller.getLoc(), SymbolRefAttr::get(calleeFuncOp), outTypes, inCallerNew);

      // Update the uses of the old call results to use the new call results
      index = 0;
      for (auto argIndex : OutargIndeices) {
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