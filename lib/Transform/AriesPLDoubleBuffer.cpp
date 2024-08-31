#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
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

struct AriesPLDoubleBuffer 
       : public AriesPLDoubleBufferBase<AriesPLDoubleBuffer> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
  
    if (!PLDoubleBuffer(mod))
      signalPassFailure();
  }

private:
  // Calculate the current iteration and 
  // create the conditions to control double buffer
  void createControl(OpBuilder builder, scf::IfOp& ifOp, arith::CmpIOp& cond1,
                     SmallVector<AffineForOp, 6> reverseBand){
    auto loc = builder.getUnknownLoc();
    auto context = builder.getContext();
    auto innerloop = reverseBand[0];
    // Calculate the current iteration to control the double buffer
    SmallVector<AffineMap, 4> affineMaps;
    SmallVector<Value, 4> ivs;
    for(auto loop : reverseBand){
      affineMaps.push_back(loop.getUpperBoundMap());
      ivs.push_back(loop.getInductionVar());
    }
    AffineExpr finalExpr = builder.getAffineDimExpr(0);
    for (unsigned i = 1; i < ivs.size(); ++i) {
      auto upExpr = affineMaps[i-1].getResult(0);
      finalExpr = finalExpr + builder.getAffineDimExpr(i) * upExpr;
    }
    auto affineMap = AffineMap::get(ivs.size(), 0, finalExpr, context);
    builder.setInsertionPointToStart(innerloop.getBody());
    auto twoAttr = builder.getIndexAttr(2);
    auto twoValue = builder.create<arith::ConstantOp>(loc, twoAttr);
    auto zeroAttr = builder.getIndexAttr(0);
    auto zeroValue = builder.create<arith::ConstantOp>(loc, zeroAttr);
    builder.setInsertionPointAfter(zeroValue);
    auto applyOp = builder.create<AffineApplyOp>(loc, affineMap, ivs);
    builder.setInsertionPointAfter(applyOp);
    auto cond = builder.create<arith::CmpIOp>(loc, arith::CmpIPredicate::eq, 
                                              applyOp, zeroValue);
    builder.setInsertionPointAfter(cond);
    auto iter = builder.create<arith::RemSIOp>(loc, applyOp, twoValue);
    builder.setInsertionPointAfter(iter);
    cond1 = builder.create<arith::CmpIOp>(loc, arith::CmpIPredicate::eq, iter, 
                                          zeroValue);
    // Create double buffer control logic
    auto i1Type = builder.getI1Type();
    TypeRange types = {i1Type, i1Type};
    SmallVector<Location, 2> locs = {loc, loc};
    builder.setInsertionPointAfter(cond1);
    ifOp  = builder.create<scf::IfOp>(loc, types, cond, true, true);
    auto trueAttr = builder.getBoolAttr(true);
    auto falseAttr = builder.getBoolAttr(false);
    builder.setInsertionPointToStart(&ifOp.getThenRegion().front());
    auto enable0_0 = builder.create<arith::ConstantOp>(loc, trueAttr);
    auto enable1_0  = builder.create<arith::ConstantOp>(loc, falseAttr);
    SmallVector<Value> results0({enable0_0, enable1_0});
    builder.create<scf::YieldOp>(loc, results0);
    builder.setInsertionPointToStart(&ifOp.getElseRegion().front());
    auto enable0_1 = builder.create<arith::ConstantOp>(loc, trueAttr);
    auto enable1_1 = builder.create<arith::ConstantOp>(loc, trueAttr);
    SmallVector<Value> results1({enable0_1, enable1_1});
    builder.create<scf::YieldOp>(loc, results1);
  }
  // Add the enable singal to each module and split them into functions
  void funcSplit(OpBuilder builder, FuncOp func, scf::IfOp ifOp,
                 SmallVector<AffineForOp, 2> forOps,
                 SmallVector<CallOp, 2>& calls){
    auto loc = builder.getUnknownLoc();
    unsigned index = 0;
    for(auto forOp : forOps){
      auto result = ifOp.getResult(index);
      // Collect the livein arguments of the forOp
      SmallVector<Value> inputs(forOp.getOperands());
      auto liveness = Liveness(forOp);
      for (auto livein: liveness.getLiveIn(forOp.getBody()))
        if (!forOp->isProperAncestor(livein.getParentBlock()->getParentOp()))
          inputs.push_back(livein);
      inputs.push_back(result);
      builder.setInsertionPoint(func);
      auto symbol = func.getSymName();
      auto moduleAttr = dyn_cast<IntegerAttr>(forOp->getAttr("module"));
      auto newName = symbol.str() + "_" + std::to_string(moduleAttr.getInt());
      forOp->removeAttr("module");
      auto funcType 
           = builder.getFunctionType(ValueRange(inputs), TypeRange({}));
      auto newfunc = builder.create<FuncOp>(loc, newName, funcType);
      auto destBlock = newfunc.addEntryBlock();
      builder.setInsertionPointToEnd(destBlock);
      auto returnOp = builder.create<ReturnOp>(builder.getUnknownLoc());
      // Create enable signal to control the whole kernel
      builder.setInsertionPoint(returnOp);
      auto numArg = destBlock->getNumArguments();
      auto lastArg = destBlock->getArgument(numArg-1);
      auto newIfOp = builder.create<scf::IfOp>(loc, lastArg, false);
      auto destIfblock = &newIfOp.getThenRegion().front();
      auto yiledOp = destIfblock->getTerminator();
      // Create the function CallOp
      builder.setInsertionPoint(forOp);
      auto call = builder.create<CallOp>(loc, newfunc, inputs);
      calls.push_back(call);
      // Move the entire block of forOp before the yiledOp
      forOp->moveBefore(yiledOp);
      // Update the references in the newfunc after move
      auto argNum = destBlock->getNumArguments();
      for (unsigned i = 0; i < argNum; i++) {
        auto sourceArg = inputs[i];
        auto destArg = destBlock->getArgument(i);
        sourceArg.replaceUsesWithIf(destArg,[&](OpOperand &use){
            return newfunc->isProperAncestor(use.getOwner());
        });
      }
      index++;
    }
  }
  // According to the condition, create ping pong buffers to ensure no arguments
  // of the two functions in common
  void createDoubleBuffer(OpBuilder builder, arith::CmpIOp cond,
                          SmallVector<CallOp, 2> calls,
                          SmallVector<AffineForOp, 6> reverseBand){
    auto loc = builder.getUnknownLoc();
    auto callPing = calls[0];
    auto callPong = calls[1];
    SmallVector<AllocOp, 4> allocPongs;
    // Tranverse the func.call and create ping pong buffers
    SmallVector<unsigned, 4> pingIdxs;
    SmallVector<unsigned, 4> pongIdxs;
    unsigned pingIdx = 0;
    for(auto operand : callPing.getOperands()){
      auto defineOp = operand.getDefiningOp();
      if(!defineOp){
        pingIdx++;
        continue;
      }
      auto allocPing = dyn_cast<AllocOp>(defineOp);
      if(!allocPing){
        pingIdx++;
        continue;
      }
      pingIdxs.push_back(pingIdx++);
      auto result = allocPing.getResult();
      auto memref = dyn_cast<MemRefType>(allocPing.getType());
      auto intAttr = dyn_cast<IntegerAttr>(memref.getMemorySpace());
      if(intAttr && intAttr.getInt() == (int)MemorySpace::L2){
        unsigned pongIdx = 0;
        for(auto user : result.getUsers()){
          if(user != callPong){
            pongIdx++;
            continue;
          }
          auto allocPong = dyn_cast<AllocOp>(allocPing->clone());
          allocPongs.push_back(allocPong);
          pongIdxs.push_back(pongIdx++);
          builder.setInsertionPointAfter(allocPing);
          builder.insert(allocPong);
          break;
        }
      }
    }
    // Create control logic for func.calls
    builder.setInsertionPoint(callPing);
    auto ifOp = builder.create<scf::IfOp>(loc, cond, true);
    auto destThenblock = &ifOp.getThenRegion().front();
    auto thenYiledOp = destThenblock->getTerminator();
    auto destElseblock = &ifOp.getElseRegion().front();
    auto elseYiledOp = destElseblock->getTerminator();
    // Create func.call in thenblock
    auto thenCallPing = dyn_cast<CallOp>(callPing->clone());
    builder.setInsertionPoint(thenYiledOp);
    builder.insert(thenCallPing);
    unsigned index = 0;
    auto thenCallPong = dyn_cast<CallOp>(callPong->clone());
    for(auto idx: pongIdxs){
      auto result = allocPongs[index++].getResult();
      thenCallPong.setOperand(idx, result);
    }
    builder.insert(thenCallPong);
    // Create func.call in elseblock
    index = 0;
    builder.setInsertionPoint(elseYiledOp);
    auto elseCallPing = dyn_cast<CallOp>(callPing->clone());
    for(auto idx: pingIdxs){
      auto result = allocPongs[index++].getResult();
      elseCallPing.setOperand(idx, result);
    }
    builder.insert(elseCallPing);
    auto elseCallPong = dyn_cast<CallOp>(callPong->clone());
    builder.insert(elseCallPong);
    callPing.erase();
    callPong.erase();
    // Need to call pong func once more outside of the outerloop
    /* TODO Handle non-constant outerloops
    SmallVector<Value, 4> operands;
    auto context = builder.getContext();
    auto newExpr = builder.getAffineConstantExpr(1);
    for (auto loop : reverseBand) {
      auto upperBound = loop.getUpperBound();
      auto upperBOundMap = upperBound.getMap();
      auto mapOps = upperBound.getOperands();
      newExpr = newExpr * upperBOundMap.getResult(0);
      operands.insert(operands.end(), mapOps.begin(), mapOps.end());
    }
    auto map = AffineMap::get(operands.size(), 0, newExpr, context);
    builder.setInsertionPointAfter(outerloop);
    auto appliedResult = builder.create<AffineApplyOp>(loc, map, operands);
    */
    auto outerloop = reverseBand[reverseBand.size()-1];
    unsigned upBound = 1;
    auto trueAttr = builder.getBoolAttr(true);
    builder.setInsertionPointAfter(outerloop);
    auto trueVal = builder.create<arith::ConstantOp>(loc, trueAttr);
    for(auto loop : reverseBand){
      if(!loop.hasConstantUpperBound())
        assert("When doing double buffer, there are non-constant loops\n");
      upBound = upBound * loop.getConstantUpperBound();
    }
    builder.setInsertionPointAfter(trueVal);
    CallOp newCallPong;
    if(upBound%2==0){
      newCallPong = dyn_cast<CallOp>(thenCallPong->clone());
      newCallPong.setOperand(thenCallPong.getNumOperands()-1, trueVal);
    }else{
      newCallPong = dyn_cast<CallOp>(elseCallPong->clone());
      newCallPong.setOperand(elseCallPong.getNumOperands()-1, trueVal);
    }
    builder.insert(newCallPong);
  }
  bool PLDoubleBuffer (ModuleOp mod) {
    auto builder = OpBuilder(mod);
    mod.walk([&](FuncOp func){
      auto rootLoop = getFirstOpOfType<AffineForOp>(func.getBody());
      if(!func->hasAttr("adf.pl") || !rootLoop)
        return WalkResult::advance();
      SmallVector<AffineForOp, 6> tileBand;
      getPerfectlyNestedLoops(tileBand,rootLoop);
      auto innerloop = tileBand[tileBand.size()-1];
      if(!innerloop->hasAttr("Array_Partition"))
        return WalkResult::advance();
      auto reverseBand = tileBand;
      std::reverse(reverseBand.begin(), reverseBand.end());
      // Check if there exists and any exists two forOps marked by module
      SmallVector<AffineForOp, 2> forOps;
      for(auto forOp : innerloop.getOps<AffineForOp>())
        if(forOp->hasAttr("module"))
          forOps.push_back(forOp);
      if(!forOps.size())
        return WalkResult::advance();
      else if(forOps.size()!=2){
        assert("The number of module is not two, double buffer doesn't apply");
        return WalkResult::advance();
      }

      scf::IfOp ifOp;
      arith::CmpIOp cond;
      SmallVector<CallOp, 2> calls;
      createControl(builder, ifOp, cond, reverseBand);
      funcSplit(builder, func, ifOp, forOps, calls);
      createDoubleBuffer(builder, cond, calls, reverseBand);
      
      return WalkResult::advance();
    });
    return true;
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesPLDoubleBufferPass() {
  return std::make_unique<AriesPLDoubleBuffer>();
}

} // namespace aries
} // namespace mlir