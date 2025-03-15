#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "llvm/Support/Debug.h"
#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "aries/Dialect/ADF/ADFDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/Affine/Analysis/LoopAnalysis.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/Passes.h"
#include "mlir/Dialect/Affine/Passes.h"
#include "mlir/Pass/PassManager.h"

using namespace mlir;
using namespace aries;
using namespace adf;
using namespace mlir::memref;

namespace {

struct AriesDependencyExtract 
                  : public AriesDependencyExtractBase<AriesDependencyExtract> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    // 核心函数是DependencyExtract
    if (!DependencyExtract(mod))
      signalPassFailure();
  }

private:
  // Collect dmaOp that access the external mem
  // 收集所有的dma操作，均是dma读或写外部内存
  void DMACollect(AffineForOp forop, SmallVector<DmaOp, 4>& dmaOps){
    forop.walk([&](DmaOp dma){
      auto dmaSrc = dma.getSrc();
      auto dmaDst = dma.getDst();
      auto SrcSpace 
          = dyn_cast<MemRefType>(dmaSrc.getType()).getMemorySpaceAsInt();
      auto DstSpace 
          = dyn_cast<MemRefType>(dmaDst.getType()).getMemorySpaceAsInt();

      // 判断MemRefType的MemorySpace是否为L1
      // 先前的pass大上的tag
      if(SrcSpace && SrcSpace == (int)mlir::aries::adf::MemorySpace::L1){
        // 原始数据在L1中，目的地不在L1中
        // cell输出数据
        if (!DstSpace || DstSpace!=(int)mlir::aries::adf::MemorySpace::L1) {
          dmaOps.push_back(dma);
        }
      }else if(DstSpace && DstSpace == (int)mlir::aries::adf::MemorySpace::L1){
        if (!SrcSpace || SrcSpace!=(int)mlir::aries::adf::MemorySpace::L1) {
          // 原始数据不在L1中，目的地在L1中
          // cell输入数据
          dmaOps.push_back(dma);
        }
      }
    });
  }

  //Check if the collected dmaOp write to the external mem
  // 判断dma操作是否是写操作，即目的地不能是L1
  bool IsWrite(DmaOp dma){
    auto dmaDst = dma.getDst();
    auto DstSpace 
        = dyn_cast<MemRefType>(dmaDst.getType()).getMemorySpaceAsInt();
    if(!DstSpace || DstSpace!=(int)mlir::aries::adf::MemorySpace::L1)
      return true;
    else
      return false;
  }

  // This function extracts the loop-carried flow dependencies 
  // There is a dependency if: 1) two accesses are to the same memory
  //                           2) one of the accesses is a write
  //                           3) precedence constraints
  // This is an experimental pass which should be replaced by mathmatical
  // formulation e.g. polyhedral model                      
  // 实验性pass，后续可能会被多面体模型替代
  bool DependencyExtract (ModuleOp mod) {
    auto builder = OpBuilder(mod);
    // Tranverse all the adf.func
    for (auto func : mod.getOps<FuncOp>()) {
      // 只对adf.func进行处理
      if(!func->hasAttr("adf.func"))
        continue;
      PassManager pm(func->getContext());
      pm.addPass(createCSEPass());
      pm.addPass(createCanonicalizerPass());
      if (failed(pm.run(func)))
        return false;

      SmallVector<AffineForOp, 6> band;

      // Find the CellOp
      // 获取adf.func中的第一个cell op
      CellOp cellOp = getFirstOpOfType<CellOp>(func.getBody());

      // 有cellOp，则从cellOp中获取loop band
      // 否则从func中获取loop band
      // 取决于是newTiling还是tiling
      // band中存储所有的affine for op
      if(cellOp)
        getNestedLoopBand(cellOp.getBody(), band);
      else
        getNestedLoopBand(func.getBody(), band);

      // 获取最内层的loop
      auto innerloop = band[band.size()-1];
      SmallVector<DmaOp, 4> dmaOps;
      // 在inner loop中，收集所有的dma操作
      // inner cell中，从L1输入数据到其他Level或是从其他Level到L1输出数据的dma操作需要记录
      DMACollect(innerloop, dmaOps);

      // Simplify the loop structure before analyze the dependency.
      // There won't be any nested affine.apply ops
      PassManager pm1(func.getContext(), "func.func");
      // 化简affine结构
      pm1.addPass(createSimplifyAffineStructuresPass());
      (void)pm1.run(func);

      auto indexType = builder.getIndexType();
      auto zeroAttr = builder.getIntegerAttr(indexType, 0);
      // Find DmaWrite -> DmaRead pair that access the same mem
      for (auto dmaWrite : dmaOps){
        unsigned opIndex=0;
        if (!IsWrite(dmaWrite))
          continue;

        // 获取destination的memref的Memref，offsets，sizes，strides
        auto dst = dmaWrite.getDst();
        auto dstOffsets = dmaWrite.getDstOffsets();
        auto dstSizes   = dmaWrite.getDstSizes(); 
        auto dstStrides = dmaWrite.getDstStrides();
        for (auto dmaRead : dmaOps){
          if (IsWrite(dmaRead))
            continue;
          auto src = dmaRead.getSrc();
          auto srcOffsets = dmaRead.getSrcOffsets();
          auto srcSizes = dmaRead.getSrcSizes(); 
          auto srcStrides = dmaRead.getSrcStrides();

          // 判断是否有read和write的依赖
          if(src != dst || srcOffsets != dstOffsets 
                        || srcSizes   != dstSizes
                        || srcStrides != dstStrides)
            continue;
          if(opIndex>0){
            // 只能有一个flow dependency
            llvm::errs() 
                << "More than one flow dependencies found\n";
            return false;
          }
          opIndex++;
          // If the loop variable is not included inside the access then
          // it will cause loop-carried flow dependency
          // 即如果loop variable不在访问内部，则会导致loop-carried flow dependency
          unsigned bandIndex=0;
          bool initial_flag = true;
          // 分析循环变量是否影响DMA访问的偏移量。
          // 如果循环变量未用于访问偏移，则可能导致跨迭代的流依赖，此时在循环上标记flow属性，并在DMA操作上标记write和read属性。
          for(auto loop: band){
            auto vi = loop.getInductionVar();
            bool flag =false;
            Value temp;
            for(auto user: vi.getUsers()){
              // 判断user里面是否有dma操作
              if(auto dmaOp = dyn_cast<DmaOp>(user) ){
                temp = vi;
                flag =true;
              }else if(auto applyop = dyn_cast<AffineApplyOp>(user)){
                temp = applyop.getResult();
                flag =true;
              }
              if(!flag)
                continue;

              // 判断是否有flow dependency
              if(std::find(srcOffsets.begin(), srcOffsets.end(), temp) 
                 !=  srcOffsets.end())
                continue;
              if(bandIndex>1){
                llvm::errs() 
                  << "More than two non-single parallel loops lead"
                  << "to a flow dependency\n";
                return false;
              }else if(initial_flag){
              //Only set one loop, but can monitor more non-single loops
                if(getConstantTripCount(loop)>1){
                  initial_flag = false;
                  // 设置flow属性，即loop-carried flow dependency
                  // 设置write属性，即写操作
                  // 设置read属性，即读操作
                  loop->setAttr("flow", builder.getUnitAttr());
                  dmaWrite->setAttr("write", zeroAttr);
                  dmaRead->setAttr("read", zeroAttr);
                  bandIndex++; 
                }
              }else {
                if(getConstantTripCount(loop)>1)
                  bandIndex++; 
              }
            }
          }
        }
      }
    }
    return true;
  }

};
} // namespace



namespace mlir {
namespace aries {

// 做提取依赖关系的pass工作
// ​该Pass用于检测硬件加速器（如FPGA/ASIC）代码中的循环携带流依赖（Loop-Carried Flow Dependency）​，并在相关位置标记属性，为后续优化（如流水线同步、循环并行化）提供依据。
std::unique_ptr<Pass> createAriesDependencyExtractPass() {
  return std::make_unique<AriesDependencyExtract>();
}

} // namespace aries
} // namespace mlir