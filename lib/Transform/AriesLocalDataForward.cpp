#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "llvm/Support/Debug.h"
#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "aries/Dialect/ADF/ADFDialect.h"

using namespace mlir;
using namespace aries;
using namespace adf;
using namespace mlir::func;

struct ConnectForward: public OpRewritePattern<ConnectOp>{
ConnectForward(MLIRContext *context)
      : OpRewritePattern<ConnectOp>(context, /*benefit=*/1) {}

  LogicalResult matchAndRewrite(
    ConnectOp op, PatternRewriter &rewriter) const override {

    auto ConSrc = op.getSrc();
    auto ConDst = op.getDst();
    
    if(op->getAttr("finish")){
      rewriter.eraseOp(op);
      return success();
    }

    if(auto readAttr = op->getAttr("read")){
      auto intRAttr = dyn_cast<IntegerAttr>(readAttr);
      auto RIndex = intRAttr.getInt();
      for(auto use: ConSrc.getUsers()){
        if(auto conop = dyn_cast<ConnectOp>(use)){
          if(auto writeAttr = conop->getAttr("write")){
            auto intWAttr = dyn_cast<IntegerAttr>(writeAttr);
            auto WIndex = intWAttr.getInt();
            if(WIndex == RIndex -1){
              auto src = conop.getSrc();
              auto dst = op.getDst();
              rewriter.setInsertionPointAfter(op);
              rewriter.replaceOpWithNewOp<ConnectOp>(op,src,dst);
              conop->removeAttr("write");
              conop->setAttr("finish",rewriter.getUnitAttr());
              return success();
            }
          }
        }
      }
    }else if(auto writeAttr = op->getAttr("write")){
      auto intWAttr = dyn_cast<IntegerAttr>(writeAttr);
      auto WIndex = intWAttr.getInt();
      for(auto use: ConDst.getUsers()){
        if(auto conop = dyn_cast<ConnectOp>(use)){
          if(auto readAttr = conop->getAttr("read")){
            auto intRAttr = dyn_cast<IntegerAttr>(readAttr);
            auto RIndex = intRAttr.getInt();
            if(WIndex == RIndex - 1){
              auto src = op.getSrc();
              auto dst = conop.getDst();
              rewriter.setInsertionPointAfter(conop);
              rewriter.replaceOpWithNewOp<ConnectOp>(op, src,dst);
              conop->removeAttr("read");
              conop->setAttr("finish",rewriter.getUnitAttr());
              return success();
            }
          }
        }
      }
    }
    return success();
  }
};

namespace {

struct AriesLocalDataForward : public AriesLocalDataForwardBase<AriesLocalDataForward> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";
  
    if (!LocalDataForward(mod,topFuncName))
      signalPassFailure();
  }

private:

  bool LocalDataForward (ModuleOp mod, StringRef topFuncName) {
    MLIRContext &context = getContext();
    RewritePatternSet patterns(&context);

    FuncOp topFunc;
    if(!topFind(mod, topFunc, topFuncName)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }

    patterns.add<ConnectForward>(patterns.getContext());

    (void)applyPatternsAndFoldGreedily(mod, std::move(patterns));

    return true;
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesLocalDataForwardPass() {
  return std::make_unique<AriesLocalDataForward>();
}

} // namespace aries
} // namespace mlir