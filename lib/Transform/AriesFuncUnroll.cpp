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

struct AriesFuncUnroll : public AriesFuncUnrollBase<AriesFuncUnroll> {
public:
  void runOnOperation() override {
    llvm::outs() << "Create the FuncUnroll Pass\n" ;
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";    
  
    if (!FuncUnroll(mod,topFuncName))
      signalPassFailure();
  }

private:
  bool FuncUnroll (ModuleOp mod,StringRef topFuncName) {
    auto b = OpBuilder(mod);
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

  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesFuncUnrollPass() {
  return std::make_unique<AriesFuncUnroll>();
}

} // namespace aries
} // namespace mlir