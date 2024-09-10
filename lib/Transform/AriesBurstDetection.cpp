#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Analysis/Liveness.h"
#include "mlir/Dialect/Affine/Utils.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/Dialect/Affine/Analysis/LoopAnalysis.h"
#include "llvm/Support/Debug.h"
#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "aries/Dialect/ADF/ADFDialect.h"

using namespace mlir;
using namespace aries;
using namespace adf;
using namespace mlir::memref;
using namespace mlir::func;
using namespace mlir::affine;

namespace {

struct AriesBurstDetection : public AriesBurstDetectionBase<AriesBurstDetection> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    
    if (!BurstDetection(mod))
      signalPassFailure();
  }

private:
  void getMemInfo(AffineForOp innerLoop, Value memref, AffineMap map,
                  SmallVector<Operation> accessOps, 
                  SmallVector<Operation>& ops,
                  SmallVector<Value>& operands){
    for (auto& op: innerLoop.getBody()->getOperations()){
      if (auto read = dyn_cast<AffineReadOpInterface>(op)) {
        memref = read.getMemRef();
        auto type = dyn_cast<MemRefType>(memref.getType());
        if(auto memSpace = type.getMemorySpace()){
          auto intSpace = dyn_cast<IntegerAttr>(memSpace);
          if(intSpace && intSpace.getInt()==(int)MemorySpace::L3){
            map = read.getAffineMap();
            operands = read.getMapOperands();
            accessOps.push_back(op);
          }else{
            ops.push_back(op);
          }
        }else{
          ops.push_back(op);
        }
      }else if (auto write = dyn_cast<AffineWriteOpInterface>(op)) {
        memref = write.getMemRef();
        auto type = dyn_cast<MemRefType>(memref.getType());
        if(auto memSpace = type.getMemorySpace()){
          auto intSpace = dyn_cast<IntegerAttr>(memSpace);
          if(intSpace && intSpace.getInt()==(int)MemorySpace::L3){
            map = write.getAffineMap();
            operands = write.getMapOperands();
            accessOps.push_back(op);
          }else{
            ops.push_back(op);
          }
        }else{
          ops.push_back(op);
        }
      }else{
        ops.push_back(op);
      }
    }
  }

  // This is a very specific optimization only for the assumed situations
  // This function merge loops together to increse burst length following 
  // these criteria
  // 1) Both are perfect nested loops and have the same number of loops
  // 2) The step of the innermost loop should be 1
  // 3) All the loops share the same lb, ub and step,
  // 4) The innermost loop should be used in the last dim of mem with coeff=1
  // 5) They share the same memory access relation excepet the last dim
  // 6) There last dim has constant distance which equals to the ub of the 
  //    innermost loop 
  // 7）Merge the loops and memory accesses together, create affine.if for other
  //    operations and erase the srcLoop
  void loopMerge(AffineForOp& srcLoop, AffineForOp& dstLoop){
    // Step 1-2: Get the nested for loops and check the loop number and step
    SmallVector<AffineForOp, 6> srcBand;
    SmallVector<AffineForOp, 6> dstBand;
    getPerfectlyNestedLoops(srcBand, srcLoop);
    getPerfectlyNestedLoops(dstBand, srcLoop);
    auto srcSize = srcBand.size();
    auto dstSize = dstBand.size();
    auto srcInner = srcBand[srcSize-1];
    auto dstInner = dstBand[dstSize-1];
    //Check if both are perfectly nested loops and the number of loops is same
    AffineForOp op; //This is a placeholder
    if(getLoopNum(srcInner, op)!=0 || getLoopNum(dstInner, op)!=0 ||
       srcInner.getStepAsInt() != 1 || dstInner.getStepAsInt() != 1 ||
       srcSize != dstSize )
      return;
    
    // Step 3: Check if are the loops share the same lbs, ubs and steps
    for(unsigned i = 0; i < srcSize; i++){
      auto srcForOp = srcBand[i];
      auto dstForOp = srcBand[i];
      if(srcForOp.getConstantLowerBound()!=dstForOp.getConstantLowerBound() ||
         srcForOp.getConstantUpperBound()!=dstForOp.getConstantUpperBound() ||
         srcForOp.getStepAsInt()!=dstForOp.getStepAsInt())
         return;
    }
    
    // Step 4: Check the coefficients of the inner loop used in memory access
    // Get the access of L3 mem first
    Value srcMemref, dstMemref;
    AffineMap srcMap, dstMap;
    SmallVector<Operation> srcAccessOps;
    SmallVector<Operation> dstAccessOps;
    SmallVector<Value> srcOperands, dstOperands;
    SmallVector<Operation> srcOps, dstOps;
    getMemInfo(srcInner, srcMemref, srcMap, srcAccessOps, srcOps, srcOperands);
    getMemInfo(dstInner, dstMemref, dstMap, dstAccessOps, dstOps, dstOperands);

  }

  // This pass assumes that function marked by "L3" only plays
  // the role of accessing the L3 memory and push/pop the data to the stream.
  // TODO: This pass is a work around, and need to be rigourously implemented
  // by using polyhedral analysis
  bool BurstDetection (ModuleOp mod) {
    auto builder = OpBuilder(mod);
    auto loc = builder.getUnknownLoc();
    auto indexType = builder.getIndexType();
    auto oneAttr = builder.getIntegerAttr(indexType, 1);
    // Detect if it is the pl function that interacts with L3 memory
    // This pass tries to merge the memory access together, in order to
    // increase the burst length.
    mod.walk([&](FuncOp func){
      if(!func->hasAttr("adf.pl") || !func->hasAttr("L3"))
        return WalkResult::advance();
      SmallVector<AffineForOp> mergeLoops;
      func.walk([&](AffineForOp op){
        if(op->hasAttr("merge")){
          mergeLoops.push_back(op);
        }
      });
    });
    return true;
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesBurstDetectionPass() {
  return std::make_unique<AriesBurstDetection>();
}

} // namespace aries
} // namespace mlir