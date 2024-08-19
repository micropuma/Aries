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
    StringRef topFuncName = "top_func";
  
    if (!DependencyExtract(mod,topFuncName))
      signalPassFailure();
  }

private:
  // Collect dmaOp that access the external mem
  void DMACollect(AffineForOp forop, SmallVector<DmaOp, 4>& dmaOps){
    forop.walk([&](DmaOp dma){
      auto dmaSrc = dma.getSrc();
      auto dmaDst = dma.getDst();
      auto SrcSpace 
          = dmaSrc.getType().dyn_cast<MemRefType>().getMemorySpaceAsInt();
      auto DstSpace 
          = dmaDst.getType().dyn_cast<MemRefType>().getMemorySpaceAsInt();

      if(SrcSpace && SrcSpace == (int)mlir::aries::adf::MemorySpace::L1){
        if (!DstSpace || DstSpace!=(int)mlir::aries::adf::MemorySpace::L1) {
          dmaOps.push_back(dma);
        }
      }else if(DstSpace && DstSpace == (int)mlir::aries::adf::MemorySpace::L1){
        if (!SrcSpace || SrcSpace!=(int)mlir::aries::adf::MemorySpace::L1) {
          dmaOps.push_back(dma);
        }
      }
    });
  }

  //Check if the collected dmaOp write to the external mem
  bool IsWrite(DmaOp dma){
    auto dmaDst = dma.getDst();
    auto DstSpace 
        = dmaDst.getType().dyn_cast<MemRefType>().getMemorySpaceAsInt();
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
  bool DependencyExtract (ModuleOp mod, StringRef topFuncName) {
    auto builder = OpBuilder(mod);
    FuncOp topFunc;
    if(!topFind(mod, topFunc, topFuncName)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }

    PassManager pm(&getContext());
    pm.addPass(createCSEPass());
    pm.addPass(createCanonicalizerPass());

    if (failed(pm.run(topFunc))) {
      return false;
    }

    SmallVector<AffineForOp, 6> band;

    // Find the CellOp
    // TODO: Handle Multiple CellOps
    CellOp cellOp = getFirstOpOfType<CellOp>(topFunc.getBody());

    if(cellOp)
      getLoopBand(cellOp.getBody(), band);
    else
      getLoopBand(topFunc.getBody(), band);
    
    auto innerloop = band[band.size()-1];
    SmallVector<DmaOp, 4> dmaOps;
    DMACollect(innerloop, dmaOps);

    // Find DmaWrite -> DmaRead pair that access the same mem
    for (auto dmaWrite : dmaOps){
      unsigned opIndex=0;
      if (IsWrite(dmaWrite)){
        auto dst = dmaWrite.getDst();
        auto dstOffsets = dmaWrite.getDstOffsets();
        auto dstSizes   = dmaWrite.getDstSizes(); 
        auto dstStrides = dmaWrite.getDstStrides();
        for (auto dmaRead : dmaOps){
          if (!IsWrite(dmaRead)){
            auto src = dmaRead.getSrc();
            auto srcOffsets = dmaRead.getSrcOffsets();
            auto srcSizes = dmaRead.getSrcSizes(); 
            auto srcStrides = dmaRead.getSrcStrides(); 
            if(src == dst && srcOffsets == dstOffsets 
                          && srcSizes   == dstSizes
                          && srcStrides == dstStrides){
              // If the loop variable is not included inside the access then
              // it will cause loop-carried flow dependency
              unsigned bandIndex=0;
              for(auto loop: band){
                auto vi = loop.getInductionVar();
                bool flag =false;
                Value temp;
                for(auto user: vi.getUsers()){
                  if(auto dmaOp = dyn_cast<DmaOp>(user) ){
                    temp = vi;
                    flag =true;
                  }else if(auto applyop = dyn_cast<AffineApplyOp>(user)){
                    temp = applyop.getResult();
                    flag =true;
                  }
                  if(flag){
                    if(std::find( srcOffsets.begin(), srcOffsets.end(), temp) 
                       ==  srcOffsets.end())
                    if(getConstantTripCount(loop)>1){
                      if(opIndex>0){
                        llvm::outs() 
                            << "More than one flow dependencies found\n";
                        return false;
                      }
                      if(bandIndex>0){
                        llvm::outs() 
                            << "More than one loops leads to flow dependency\n";
                        return false;
                      }  
                      loop->setAttr("flow", builder.getUnitAttr());
                      dmaWrite->setAttr("write", builder.getUnitAttr());
                      dmaRead->setAttr("read", builder.getUnitAttr());
                      opIndex++;
                      bandIndex++;
                    }
                  }
                }
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

std::unique_ptr<Pass> createAriesDependencyExtractPass() {
  return std::make_unique<AriesDependencyExtract>();
}

} // namespace aries
} // namespace mlir