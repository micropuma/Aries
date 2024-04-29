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

    if(!subviewHoist(topFunc, calleeFuncOp)){
      return false;
    }

    return true;
  }

  bool subviewHoist(FuncOp topFunc, FuncOp calleeFuncOp){
    unsigned num=0;
    topFunc.walk([&](CallOp call) { ++num; });
    auto flag = topFunc.walk([&](CallOp caller){
      unsigned opNum = 0;
      auto operands = caller.getOperands();
      for (auto operand: operands){
        auto defineOp = operand.getDefiningOp();
        //Check if the input arguments are constant values or memref
        if ( !(dyn_cast<arith::ConstantOp>(defineOp) || dyn_cast<MemRefType>(operand.getType())) ){
          caller->emitOpError("There is non-constant or non-mem argument in the caller function\n");
          return WalkResult::interrupt();
        }else if(dyn_cast<MemRefType>(operand.getType())){
          //Create the memref subview on the topFunc for each memref argument
          SmallVector<unsigned, 4> memOffsets;
          auto subview = dyn_cast<SubViewOp>(*calleeFuncOp.getArgument(opNum).user_begin());
          auto memSizes = subview.getStaticSizes();
          auto memStrides = subview.getStaticSizes();
          llvm::outs() << memStrides.size() << "\n";
          for (unsigned i = 0; i< memStrides.size(); i++){
            auto memOffset = subview.getDynamicOffset(i);
            llvm::outs() << memOffset << "\n";
          }
        }
        opNum++;
      }
      return WalkResult::advance();
    });
    if (flag == WalkResult::interrupt())
      return false;
    
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