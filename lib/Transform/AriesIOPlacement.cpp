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

struct AriesIOPlacement : public AriesIOPlacementBase<AriesIOPlacement> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
  
    if (!IOPlacement(mod))
      signalPassFailure();
  }

private:
  bool checkValidChal(unsigned& col, unsigned& chl, unsigned arrayIndex,
                      unsigned colFirst, std::vector<int>& tileChannel){
    auto curChl = tileChannel[arrayIndex];
    if(curChl>=0){
      col = arrayIndex + colFirst;
      chl = curChl * 2;
      tileChannel[arrayIndex] = curChl -1;
      return true;
    }
    return false;
  }

  // Here find available channel at the startPos, if not then jump forward and
  // back forward between startPos, if reach one end then only go another side
  bool findPlacement(unsigned startPos, unsigned& col, unsigned& chl, 
                     std::vector<int>& tileChannel){
    unsigned colFirst = 6;
    unsigned colLast = 44;
    if(startPos < colFirst)
      startPos = colFirst;
    if(startPos > colLast)
      startPos = colLast;
    auto firstIndex = startPos - colFirst; // Record start pos of tileChannel
    auto arrayIndex = firstIndex; //Used to move position of tileChannel
    bool direction = false; //Towards left
    bool end_flag = false;
    bool reachEnd = false;
    unsigned forwardCnt = 1;
    unsigned backwardCnt = 1;
    while(!end_flag){
      if(checkValidChal(col, chl, arrayIndex, colFirst, tileChannel))
        end_flag = true;
      else{
        // If reachEnd been marked, then one side is already full
        // Only search one direction
        if(reachEnd){
          if(direction){
            arrayIndex = firstIndex + forwardCnt;
            if(arrayIndex > colLast)
              return false;
            if(checkValidChal(col, chl, arrayIndex, colFirst, tileChannel))
              end_flag = true;
            forwardCnt++;
          }else{
            arrayIndex = firstIndex - backwardCnt;
            if(arrayIndex < colFirst)
              return false;
            if(checkValidChal(col, chl, arrayIndex, colFirst, tileChannel))
              end_flag = true;
            backwardCnt++;
          }
        }else{
          // If both sides doesn't reach end, alternatively change direction
          if(direction){
            arrayIndex = firstIndex + forwardCnt;
            if(arrayIndex > colLast){
              reachEnd = true;
              arrayIndex = firstIndex - backwardCnt;
              if(checkValidChal(col, chl, arrayIndex, colFirst, tileChannel))
                end_flag = true;
              backwardCnt++;
              direction = !direction;
            }else{
              if(checkValidChal(col, chl, arrayIndex, colFirst, tileChannel))
                end_flag = true;
              forwardCnt++;
              direction = !direction;
            }
          }else{
            arrayIndex = firstIndex - backwardCnt;
            if(arrayIndex < colFirst){
              reachEnd = true;
              arrayIndex = firstIndex + forwardCnt;
              if(checkValidChal(col, chl, arrayIndex, colFirst, tileChannel))
                end_flag = true;
              forwardCnt++;
              direction = !direction;
            }else{
              if(checkValidChal(col, chl, arrayIndex, colFirst, tileChannel))
                end_flag = true;
              backwardCnt++;
              direction = !direction;
            }
          }
        }
      }
    }
    return true;
  }
  bool IOPlacement (ModuleOp mod) {
    auto builder = OpBuilder(mod);
    auto indexType = builder.getIndexType();
    unsigned middleLine = 24;
    unsigned numTile = 39;
    unsigned numChannel = 4;
    std::vector<int> tileChannel(numTile, numChannel-1);
    auto flag = mod.walk([&](FuncOp func){
      if(!func->hasAttr("adf.cell"))
        return WalkResult::advance();
      for(auto configOp : func.getOps<ConfigPLIOOp>()){
        SmallVector<unsigned, 4> corePlace;
        auto plio = configOp.getPlio();
        int disToMid = 0;
        unsigned cnt = 0;
        for(auto use : plio.getUsers()){
          if(!dyn_cast<ConnectOp>(use))
            continue;
          auto connectOp = dyn_cast<ConnectOp>(use);
          auto src = connectOp.getSrc();
          auto dst = connectOp.getDst();
          CallOp call;
          // handle results
          if(src!=plio){
            auto defineOp = src.getDefiningOp();
            if(!defineOp || !dyn_cast<CallOp>(defineOp))
              return WalkResult::interrupt();
            call = dyn_cast<CallOp>(defineOp);
          }else{
            for(auto valUse : dst.getUsers()){
              if(!dyn_cast<CallOp>(valUse))
                continue;
              call = dyn_cast<CallOp>(valUse);
              break;
            }
          }
          auto corePlaceAttr = dyn_cast<ArrayAttr>(call->getAttr("col, row"));
          auto colAttr = corePlaceAttr[0];
          auto intAttr = dyn_cast<IntegerAttr>(colAttr);
          int colCore = intAttr.getInt();
          disToMid += colCore-middleLine;
          cnt++;
        }
        if(cnt == 0)
          return WalkResult::advance();
        unsigned col;
        unsigned chl;
        auto startPos = std::ceil(disToMid/cnt) + middleLine;
        if(!findPlacement(startPos, col, chl, tileChannel))
          return WalkResult::interrupt();
        auto colAttr = builder.getIntegerAttr(indexType, col);
        auto chlAttr = builder.getIntegerAttr(indexType, chl);
        auto arrayAttr = builder.getArrayAttr({colAttr, chlAttr});
        configOp->setAttr("col, chl", arrayAttr);
      } 
      return WalkResult::advance();
    });
    if(flag == WalkResult::interrupt())
      return false;
    return true;
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesIOPlacementPass() {
  return std::make_unique<AriesIOPlacement>();
}

} // namespace aries
} // namespace mlir