#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "aries/Dialect/ADF/ADFDialect.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/Affine/Analysis/LoopAnalysis.h"

using namespace mlir;
using namespace aries;
using namespace adf;
using namespace mlir::affine;
using namespace mlir::memref;
using namespace func;


namespace {

struct AriesFuncUnroll : public AriesFuncUnrollBase<AriesFuncUnroll> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());  
    if (!FuncUnroll(mod))
      signalPassFailure();
  }

private:
  bool FuncUnroll (ModuleOp mod) {
    auto builder = OpBuilder(mod);
    // Tranverse all the adf.func
    for (auto func : mod.getOps<FuncOp>()) {
      // 必须是针对cell op的循环unroll展开。
      if(!func->hasAttr("adf.func"))
        continue;
      // Find the CellOp
      CellOp cellOp = getFirstOpOfType<CellOp>(func.getBody());
      if(!cellOp)
        return true;
      SmallVector<AffineForOp, 6> band;
      getNestedLoopBand(cellOp.getBody(), band, true);
      SmallVector<Attribute, 3> tripCountList;
      SmallVector<unsigned, 3> indexList; //Record loop with tripcount>1
      auto indexType = builder.getIndexType();
      auto zeroAttr = builder.getIntegerAttr(indexType, 0);
      auto oneAttr = builder.getIntegerAttr(indexType, 1);
      //Start from the innermost loop
      bool flowFull = false;
      bool hasRedLoop = false;
      for (unsigned index=0; index < band.size(); index++) {
        // Keep the tripCount info after unrolling
        auto loop = band[index];

        // 检查trip count是否是静态的值
        auto tripCount = getConstantTripCount(loop);
        if(!tripCount.has_value())
          return false;
        auto tripCountVal = tripCount.value();
        if(tripCountVal>1){
          // 收集trip count大于1的循环
          auto tripCountAttr = builder.getIntegerAttr(indexType, tripCountVal);
          tripCountList.push_back(tripCountAttr);
          indexList.push_back(index);
          if (loop->hasAttr("reduction"))
            hasRedLoop = true;
        }
        // Unroll reduction loop
        // 需要显示确保dma的传输的正确性
        if (loop->getAttr("flow")){
          auto annotateFn = [](unsigned i, Operation *op, OpBuilder builder) {
            auto indexType = builder.getIndexType();
            auto valueAttr = builder.getIntegerAttr(indexType,i);
            // 对于dma op，需要打上write或者read的标记
            if (auto dmaop = dyn_cast<DmaOp>(op)){
              if(auto attr = dmaop->getAttr("write"))
                dmaop->setAttr("write", valueAttr);
              else if(auto attr = dmaop->getAttr("read"))
                dmaop->setAttr("read", valueAttr);
            }else if (auto callop = dyn_cast<CallOp>(op)){
              // Mark kernel in the reduction chain to update kernel interface
              callop->setAttr("kernel", valueAttr);
              // Mark each loop iteration info for placement
              // 将loop迭代信息打上标记
              if(auto attr = callop->getAttr("ivs")){
                auto arrayAttr = dyn_cast<ArrayAttr>(attr);
                SmallVector<Attribute, 3> newAttrList;
                for (auto oldAttr : arrayAttr){
                  newAttrList.push_back(oldAttr);
                }
                newAttrList.push_back(valueAttr);
                auto newArrayAttr = builder.getArrayAttr(newAttrList);
                callop->setAttr("ivs", newArrayAttr);
              }else{
                auto arrayAttr = builder.getArrayAttr({valueAttr});
                callop->setAttr("ivs", arrayAttr);
              }
            }
          };
          // loopUnrollFull won't mark the loops with tripcount = 1 
          // annotateFn如此使用
          // for (auto it = loopBodyBlock->begin(); it != std::next(srcBlockEnd); it++) {
          // Operation *clonedOp = builder.clone(*it, operandMap);
          // annotateFn(i, clonedOp, builder);
          // }
          if (failed(loopUnrollFull(loop, annotateFn)))
            return false;
          if(flowFull){ // Only support one reduction loop
            assert("Support only one reduction loop\n");
            return false;
          }
          if(tripCount>1)
            flowFull = true;
        }
        // Unroll non-reduction loop
        else {
          auto annotateFn = [](unsigned i, Operation *op, OpBuilder builder) {
            auto indexType = builder.getIndexType();
            auto valueAttr = builder.getIntegerAttr(indexType,i);
            // 不用显示处理dma的tag
            if (auto callop = dyn_cast<CallOp>(op)){
              // Mark kernel in the reduction chain to modify the kernel interface 
              // Mark each loop iteration info for placement
              if(auto attr = callop->getAttr("ivs")){
                auto arrayAttr = dyn_cast<ArrayAttr>(attr);
                SmallVector<Attribute, 3> newAttrList;
                for (auto oldAttr : arrayAttr){
                  newAttrList.push_back(oldAttr);
                }
                newAttrList.push_back(valueAttr);
                auto newArrayAttr = builder.getArrayAttr(newAttrList);
                callop->setAttr("ivs", newArrayAttr);
              }else{
                auto arrayAttr = builder.getArrayAttr({valueAttr});
                callop->setAttr("ivs", arrayAttr);
              }
            }
          };
          // loopUnrollFull won't mark the loops with tripcount = 1 
          if (failed(loopUnrollFull(loop, annotateFn)))
            return false;
        }
      }
      // Identify the loops that has tripcounts > 1, and complete the three dims
      // for core placement
      int listSize = (int)tripCountList.size();
      SmallVector<Attribute, 3> newTripCounts;
      if(listSize>3){
        // 目前只能支持将3D的逻辑数组映射到2D的物理AIE阵列上
        llvm::errs() << "Only 3D logic array is supported\n";
        return false;
      }else{
        // If no reduction then add zero before else append zero behind
        if(!hasRedLoop)
          newTripCounts.push_back(oneAttr);
        for(int idx=0; idx<3; idx++){
          if(idx < listSize){
            auto attr = tripCountList[idx];
            newTripCounts.push_back(attr);
          }else{
            // 显示扩充维度1
            newTripCounts.push_back(oneAttr);
          }
        }
      }
      // Mark the tripCount info for placement
      auto arrayAttr = builder.getArrayAttr(newTripCounts);
      cellOp->setAttr("tripCount", arrayAttr);
      // Add a name for each function call
      llvm::SmallVector<std::pair<StringRef, unsigned>, 4> calleeCounts;
      cellOp.walk([&](CallOp call){
        auto calleeName = call.getCallee();
        bool found = false;
        for (auto &entry : calleeCounts) {
          if (entry.first == calleeName) {
            entry.second++;
            auto valueAttr 
                 = builder.getIntegerAttr(builder.getIndexType(), entry.second);
            call->setAttr(calleeName,valueAttr);
            call->setAttr("adf.kernel",builder.getUnitAttr());
            found = true;
            // Fill the ivs attributes to three if needed
            // 将call的循环迭代维度扩充到3
            SmallVector<Attribute, 3> newAttrList;
            if(auto arrayAttr = call->getAttr("ivs")){
              auto ivArrayAttr = dyn_cast<ArrayAttr>(call->getAttr("ivs"));
              if(!hasRedLoop)
                newAttrList.push_back(zeroAttr);
              for(int idx=0; idx<3; idx++){
                if(idx < listSize){
                  auto attr = ivArrayAttr[idx];
                  newAttrList.push_back(attr);
                }else{
                  newAttrList.push_back(zeroAttr);
                }
              }
            }else{
              for(int idx=0; idx<3; idx++){
                newAttrList.push_back(zeroAttr);
              }
            }
            auto newArrayAttr = builder.getArrayAttr(newAttrList);
            call->setAttr("ivs", newArrayAttr);
            break;
          }
        }
        if (!found) {
          calleeCounts.push_back(std::make_pair(calleeName, 0));
          auto valueAttr = builder.getIntegerAttr(builder.getIndexType(), 0);
          call->setAttr(calleeName,valueAttr);
          call->setAttr("adf.kernel",builder.getUnitAttr());
          // Fill the ivs attributes to three if needed
          SmallVector<Attribute, 3> newAttrList;
          if(auto arrayAttr = call->getAttr("ivs")){
            auto ivArrayAttr = dyn_cast<ArrayAttr>(arrayAttr);
            if(!hasRedLoop)
              newAttrList.push_back(zeroAttr);
            for(int idx=0; idx<3; idx++){
              if(idx < listSize){
                auto attr = ivArrayAttr[idx];
                newAttrList.push_back(attr);
              }else{
                newAttrList.push_back(zeroAttr);
              }
            }
          }else{
            for(int idx=0; idx<3; idx++){
              newAttrList.push_back(zeroAttr);
            }
          }
          auto newArrayAttr = builder.getArrayAttr(newAttrList);
          call->setAttr("ivs", newArrayAttr);
        }
      });
      // Deal with the L1 Buffers that has the same symbol
      llvm::StringMap<int> symbolCounts;
      func.walk([&](BufferOp buffer){
        auto sybmol = buffer.getSymbol();
        int &count = symbolCounts[sybmol];
        if (count > 0) {
          std::string newSymbol = sybmol.str() + "_" + std::to_string(count);
          buffer.setSymbol(newSymbol);
        }
        count++;
      });
    }
    return true;
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