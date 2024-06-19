#include "mlir/IR/Builders.h"
#include "mlir/IR/DialectImplementation.h"
#include "aries/Dialect/ADF/ADFDialect.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"

using namespace mlir;
using namespace aries;
using namespace adf;

template<typename OpType>
struct DeadElim : public OpRewritePattern<OpType> {
  using OpRewritePattern<OpType>::OpRewritePattern;
  LogicalResult matchAndRewrite(OpType op, PatternRewriter &rewriter) const override {
    if (op.use_empty()) {
      rewriter.eraseOp(op);
      return success();
    }
    return failure();
  }
};


bool IsdstTouched (IOPushOp op){
  for (unsigned i = 0; i< op.getDst().size(); i++){
    auto dst = op.getDst()[i];
    for(auto user : dst.getUsers()){
      if(auto connectOp0 = dyn_cast<ConnectOp>(user)){
        auto connectDst0 = connectOp0.getDst();
        for(auto connectDstUser0 : connectDst0.getUsers()){
          if(auto connectOp1 = dyn_cast<ConnectOp>(connectDstUser0)){
            auto connectDst1 = connectOp1.getDst();
            for(auto connectDstUser1 : connectDst1.getUsers()){
              if(dyn_cast<IOPopOp>(connectDstUser1)){
                return true;
              }
            }
          }
        }
      }
    }
  }
  return false;
}

//TODO: This should be a pass instead of a canonicalizer
//Eliminate the pushop that share the same source and number of dst
struct IOPushElim : public OpRewritePattern<IOPushOp> {
  using OpRewritePattern<IOPushOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(IOPushOp op,
                                PatternRewriter &rewriter) const override {
                                  
    SmallVector<IOPushOp> iopushOps;
    IOPushOp firstUser = nullptr;
    for (auto &user : op.getSrc().getUses()) {
      if (auto otherOp = dyn_cast<IOPushOp>(user.getOwner())) {
        //Get the first pushop that pushes the same memref slice
        if (op.getSrc() == otherOp.getSrc() &&
            op.getSrcOffsets() == otherOp.getSrcOffsets() &&
            op.getSrcSizes() == otherOp.getSrcSizes() &&
            op.getSrcStrides() == otherOp.getSrcStrides() &&
            op.getDst().size() == otherOp.getDst().size()){
            if (!firstUser || otherOp->isBeforeInBlock(firstUser)) {
              firstUser = otherOp;
            }
            iopushOps.push_back(otherOp);
        }
      }
    }
    bool flag = false;
    for (auto iopushOp: iopushOps){
      if(iopushOp == firstUser){
        continue;
      }else{
        if(IsdstTouched(iopushOp)){
          continue;
        }else{
          for (unsigned i = 0; i< iopushOp.getDst().size(); i++){
            auto dst = iopushOp.getDst()[i];
            auto dstOther = firstUser.getDst()[i];
            dst.replaceAllUsesWith(dstOther);
          }
          iopushOp.erase();
          flag = true;
        }
      }
    }
    if (flag)
      return success();
    else
      return failure();
  }
};

void CreateGraphIOOp::getCanonicalizationPatterns(RewritePatternSet &results,
                                           MLIRContext *context) {
  results.add<DeadElim<CreateGraphIOOp>>(context);
}

void IOPushOp::getCanonicalizationPatterns(RewritePatternSet &results,
                                           MLIRContext *context) {
  results.add<IOPushElim>(context);
}