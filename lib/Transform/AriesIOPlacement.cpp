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
  AriesIOPlacement() = default;
  AriesIOPlacement(const AriesOptions &opts) {
    ChalIn=opts.OptChalIn;
    ChalOut=opts.OptChalOut;
    Offset=opts.OptOffset;
  }
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
  
    if (!IOPlacement(mod))
      signalPassFailure();
  }

private:
  bool checkValidChal(unsigned& col, unsigned& chl, int arrayIndex,
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
  bool findPlacement(unsigned numTile, unsigned startPos, unsigned midLine,
                     unsigned offset, unsigned& col, unsigned& chl, 
                     std::vector<int>& tileChannel){
    unsigned colFirst = 6;
    unsigned colLast = colFirst + (numTile-1);
    if((startPos < midLine) && (midLine - startPos) < offset)
      startPos = startPos - offset;
    else if((startPos > midLine) && (startPos - midLine) < offset)
      startPos = startPos + offset;
    if(startPos < colFirst)
      startPos = colFirst;
    if(startPos > colLast)
      startPos = colLast;
    int firstIndex = startPos - colFirst; // Record start pos of tileChannel
    int arrayIndex = firstIndex; //Used to move position of tileChannel
    bool direction = false; //Towards left
    bool end_flag = false;
    bool reachEnd = false;
    unsigned forwardCnt = 1;
    unsigned backwardCnt = 1;
    while(!end_flag){
      if(checkValidChal(col, chl, arrayIndex, colFirst, tileChannel))
        break;
      // If reachEnd been marked, then one side is already full
      // Only search one direction
      if(reachEnd){
        if(direction){
          arrayIndex++;
          if(arrayIndex > (int)(numTile-1))
            return false;
          if(checkValidChal(col, chl, arrayIndex, colFirst, tileChannel))
            end_flag = true;
        }else{
          arrayIndex--;
          if(arrayIndex < 0)
            return false;
          if(checkValidChal(col, chl, arrayIndex, colFirst, tileChannel))
            end_flag = true;
        }
      }else{
        // If both sides doesn't reach end, alternatively change direction
        if(direction){
          arrayIndex = firstIndex + forwardCnt;
          if(arrayIndex > (int)(numTile-1)){
            reachEnd = true;
            arrayIndex = numTile-1;
            if(checkValidChal(col, chl, arrayIndex, colFirst, tileChannel))
              end_flag = true;
            direction = !direction;
          }else{
            if(checkValidChal(col, chl, arrayIndex, colFirst, tileChannel))
              end_flag = true;
            forwardCnt++;
            direction = !direction;
          }
        }else{
          arrayIndex = firstIndex - backwardCnt;
          if(arrayIndex < 0){
            reachEnd = true;
            arrayIndex = 0;
            if(checkValidChal(col, chl, arrayIndex, colFirst, tileChannel))
              end_flag = true;
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
    return true;
  }
  bool IOPlacement (ModuleOp mod) {
    auto builder = OpBuilder(mod);
    auto indexType = builder.getIndexType();
    unsigned midLine = 24;
    unsigned numTile = 39;
    unsigned numInChl = ChalIn;
    unsigned numOutChl = ChalOut;
    unsigned offset = Offset; //This is to move the IO near the middle
    std::vector<int> tileInChl(numTile, numInChl-1);
    std::vector<int> tileOutChl(numTile, numOutChl-1);
    auto flag = mod.walk([&](FuncOp func){
      if(!func->hasAttr("adf.cell"))
        return WalkResult::advance();
      for(auto configOp : func.getOps<ConfigPLIOOp>()){
        SmallVector<unsigned, 4> corePlace;
        auto plio = configOp.getPlio();
        int disToMid = 0;
        int cnt = 0;
        bool inDir;
        for(auto use : plio.getUsers()){
          if(!dyn_cast<ConnectOp>(use))
            continue;
          auto connectOp = dyn_cast<ConnectOp>(use);
          auto src = connectOp.getSrc();
          auto dst = connectOp.getDst();
          CallOp call;
          if(src!=plio){
            auto defineOp = src.getDefiningOp();
            if(!defineOp || !dyn_cast<CallOp>(defineOp))
              return WalkResult::interrupt();
            inDir = false;
            call = dyn_cast<CallOp>(defineOp);
          }else{
            for(auto valUse : dst.getUsers()){
              if(!dyn_cast<CallOp>(valUse))
                continue;
              call = dyn_cast<CallOp>(valUse);
              inDir = true;
              break;
            }
          }
          if(!call)
            return WalkResult::interrupt();
          auto corePlaceAttr = dyn_cast<ArrayAttr>(call->getAttr("col, row"));
          auto colAttr = corePlaceAttr[0];
          auto intAttr = dyn_cast<IntegerAttr>(colAttr);
          int colCore = intAttr.getInt();
          disToMid += colCore-midLine;
          cnt++;
        }
        if(cnt == 0)
          return WalkResult::advance();
        unsigned col;
        unsigned chl;
        int avgToMid = std::ceil(disToMid/cnt);
        unsigned startPos = avgToMid + midLine;
        if(inDir){
          if(!findPlacement(numTile, startPos, midLine, offset, col, chl, 
                            tileInChl))
            return WalkResult::interrupt();
        }else{
          if(!findPlacement(numTile, startPos, midLine, offset, col, chl, 
                            tileOutChl))
            return WalkResult::interrupt();
        }
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

std::unique_ptr<Pass> createAriesIOPlacementPass(const AriesOptions &opts) {
  return std::make_unique<AriesIOPlacement>(opts);
}

} // namespace aries
} // namespace mlir