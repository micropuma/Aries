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

struct AriesPLIOMaterialize : public AriesPLIOMaterializeBase<AriesPLIOMaterialize> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";
  
    if (!IOMaterialize(mod,topFuncName))
      signalPassFailure();
  }

private:
  //Collect and remove the adf.cells and erase adf.io.wait
  void Preprocess(LauchCellOp lauchcell, SmallVectorImpl<CallOp>& calls){
    lauchcell.walk([&](Operation *op){
      if(auto call = dyn_cast<CallOp>(op)){
        if(call->hasAttr("adf.cell"))
          calls.push_back(call);
        call->remove();
      }else if(dyn_cast<IOWaitOp>(op) || dyn_cast<WaitLauchCellOp>(op)){
        op->erase();
      }
    });
  }

  // Detect liveinVal to construct dma func
  void ArguDetect(LauchCellOp lauchcell, SmallVectorImpl<Value> &inputs){
    //Find all the liveness within LauchCellOp
    auto liveness = Liveness(lauchcell);
    //Check each liveinVal in the block
    for (auto liveinVal: liveness.getLiveIn(&lauchcell.getBody().front())){
      //Check if the liveinVal is defined in the AffineParallelOp 
      if (!lauchcell->isProperAncestor(
                            liveinVal.getParentBlock()->getParentOp())){
        inputs.push_back(liveinVal);
      }
    }
  }

  // Create PL func.func and pl.launch. Create Callop inside pl.launch
  void PLFuncCreation(OpBuilder builder, FuncOp topFunc, FuncOp& plFunc,
                      LauchCellOp lauchcell, SmallVectorImpl<Value>& inputs){
    auto loc = builder.getUnknownLoc();
    // Define the dma function with the detected inputs as arguments
    builder.setInsertionPoint(topFunc);
    auto funcName = "dma_pl";
    auto funcType = builder.getFunctionType(ValueRange(inputs), TypeRange({}));
    plFunc = builder.create<FuncOp>(builder.getUnknownLoc(),funcName,funcType);
    plFunc->setAttr("adf.pl",builder.getUnitAttr());
    auto destBlock = plFunc.addEntryBlock();
    builder.setInsertionPointToEnd(destBlock);
    auto returnOp = builder.create<ReturnOp>(builder.getUnknownLoc());
    
    // Move the operations in the adf.cell.lauch before the returnOp
    builder.setInsertionPointToEnd(destBlock);
    auto &entryBlock = lauchcell.getBody().front();
    for(auto &op: llvm::make_early_inc_range(entryBlock)){
      if(!dyn_cast<EndLauchCellOp>(op))
        op.moveBefore(returnOp);
    }

    // Create LauchPLOp and insert the CallOp
    builder.setInsertionPointAfter(lauchcell);
    auto launchPLOp = builder.create<LauchPLOp>(loc,funcName);
    auto *cellLaunchPLBlock = builder.createBlock(&launchPLOp.getBody());
    builder.setInsertionPointToEnd(cellLaunchPLBlock);
    auto endLaunchPL = builder.create<WaitLauchPLOp>(loc);
    builder.setInsertionPoint(endLaunchPL);
    auto callop = builder.create<CallOp>(loc, plFunc, inputs);
    callop->setAttr("adf.pl",builder.getUnitAttr());

    // Update the references in the plFunc after move
    for (unsigned i = 0, num_arg = destBlock->getNumArguments(); 
         i < num_arg; ++i) {
      auto sourceArg = inputs[i];
      auto destArg = destBlock->getArgument(i);
      sourceArg.replaceUsesWithIf(destArg,[&](OpOperand &use){
          return plFunc->isProperAncestor(use.getOwner());
      });
    }
  }

  // This is a helper function that resolves the offset defined recursively by
  // nested AffineApplyOps 0) val: should be the offset of IOPush
  // 1) operands:stores all the operands related to val & not defined by Applyop
  // 2) applyops: stores all the applyops related to val
  void resolveOffset(Value val, 
                     SmallVector<Value, 4>& operands,
                     SmallVector<AffineApplyOp, 4>& applyops){
    auto definedOp = val.getDefiningOp();
    if (!definedOp){
      operands.push_back(val);
      return;
    }else if (auto applyop = dyn_cast<AffineApplyOp>(definedOp)){
      if (llvm::find(applyops, applyop) == applyops.end())
        applyops.push_back(applyop);
      for (auto operand : applyop.getOperands())
        resolveOffset(operand, operands, applyops);
    }else{
      operands.push_back(val);
      return;
    }
  }

  // Allocate L2 memory
  bool ADFPLAlloc(OpBuilder builder, FuncOp plFunc){
    auto loc = builder.getUnknownLoc();
    AffineForOp plForOp;
    plFunc.walk([&](AffineForOp forOp){
      if(forOp->hasAttr("Array_Partition"))
        plForOp = forOp;
    });
    if(!plForOp){
      llvm::outs() << "Array_Partition loop not found\n";
      return false;
    }
    // Get point loops in band
    SmallVector<AffineForOp, 6> band;
    for (auto forOp : plForOp.getOps<AffineForOp>())
      getPerfectlyNestedLoops(band, forOp);

    // Normalize point loops
    for (auto forOp : band){
      if(failed(normalizeAffineFor(forOp)))
        return false;
    }

    // Move AffineApplyOp to the innerLoop
    auto outerloop = band[0];
    auto innerLoop = band[band.size()-1];
    auto loopBody = innerLoop.getBody();
    SmallVector<AffineApplyOp, 6> applyOpsBefore;
    plForOp.walk([&](AffineApplyOp op){
      applyOpsBefore.push_back(op);
    });
    std::reverse(applyOpsBefore.begin(), applyOpsBefore.end());
    for (auto applyOp :  applyOpsBefore)
      applyOp->moveBefore(&loopBody->front());

    //////////////////////////TODO////////////////////////
    /*
    Need to handle more general cases of the offest in IOPushOp
    Now assumes this is a rectangular tiling. 
    the size of local buffer = point loop bounds * size of IOPush src
    the offset of the local buffer = point loop variable * size of IOPush src
    the stride of the local buffer = 1 
    */
    //////////////////////////////////////////////////////

    // Tranverse the IOPushOps and allocate L2 buffers
    auto flag = plForOp.walk([&](IOPushOp op){
      auto src = op.getSrc();
      auto offsets = op.getSrcOffsets();
      auto sizes = op.getSrcSizes();
      auto strides = op.getSrcStrides();
      SmallVector<int64_t, 4> sizesInt;
      unsigned rank = offsets.size();
      auto type = dyn_cast<MemRefType>(src.getType());
      SmallVector<int64_t, 4> bufSizes;
      SmallVector<AffineForOp, 4> loops;
      SmallVector<AffineApplyOp, 4> applyops;
      // Get the buffer size of each dim
      unsigned dim = 0;
      for (auto offset: offsets){
        SmallVector<Value, 4> operands;
        resolveOffset(offset,operands,applyops);
        std::reverse(applyops.begin(), applyops.end());
        for (auto operand : operands){
          // Get the trip count of the corresponding point loop
          auto loop = getForInductionVarOwner(operand);
          if (loop && llvm::find(band, loop) != band.end()) {
            SmallVector<Value, 4> foroperands;
            AffineMap map;
            getTripCountMapAndOperands(loop, &map, &foroperands);
            auto tripCount = map.getSingleConstantResult();
            if (!tripCount){
              llvm::outs() << "Involve loops with non-consant trip count!\n";
              return WalkResult::interrupt();
            }
            auto size = sizes[dim];
            auto constantOp = dyn_cast<arith::ConstantOp>(size.getDefiningOp());
            if(!constantOp)
              return WalkResult::interrupt();
            auto intAttr = dyn_cast<IntegerAttr>(constantOp.getValue());
            auto sizeInt = intAttr.getInt();
            sizesInt.push_back(sizeInt);
            bufSizes.push_back(tripCount*sizeInt);
            loops.push_back(loop);
            break;
          }
        }
        dim++;
      }

      // Allocate L2 buffer before the outer point loop
      builder.setInsertionPointToStart(&plFunc.getBody().front());
      auto allocOp 
           = builder.create<AllocOp>(loc, MemRefType::get(bufSizes,
                                     type.getElementType(), AffineMap(),
                                     (int)mlir::aries::adf::MemorySpace::L2));
      
      // Create static size and strides for dma & IOPush at the front of func
      auto indexType = builder.getIndexType();
      auto oneAttr = builder.getIntegerAttr(indexType, 1);
      auto oneValue = builder.create<arith::ConstantOp>(loc,indexType,oneAttr);
      builder.setInsertionPointAfter(oneValue);
      SmallVector<Value> localSizes;
      SmallVector<Value> localStrides;
      for(unsigned i=0; i< rank; i++){
        auto sizeAttr = builder.getIntegerAttr(indexType, sizesInt[i]);
        auto sizeValue 
             = builder.create<arith::ConstantOp>(loc, indexType, sizeAttr);
        builder.setInsertionPointAfter(sizeValue);
        localSizes.push_back(sizeValue);
        localStrides.push_back(oneValue);
      }

      // Load data from external mem to L2 buffer
      SmallVector<AffineForOp, 4> newLoops;
      builder.setInsertionPoint(outerloop);
      // Clone buffer related loops
      for (auto loop : loops){
        auto newForOp 
             = builder.create<AffineForOp>(loc,
                                           loop.getLowerBoundOperands(),
                                           loop.getLowerBoundMap(),
                                           loop.getUpperBoundOperands(),
                                           loop.getUpperBoundMap());
        newLoops.push_back(newForOp);
        builder.setInsertionPointToStart(newForOp.getBody());
      }
      // Create affine ApplyOps inside the innermost new loop
      SmallVector<AffineApplyOp, 4> newApplyops;
      SmallVector<Value, 4> nestedOperands;
      SmallVector<unsigned, 4> ApplyOpIndex;
      for (auto applyop : applyops){
        auto newApplyop = builder.create<AffineApplyOp>(loc, 
                                                        applyop.getAffineMap(),
                                                        applyop.getOperands());
        newApplyops.push_back(newApplyop);
        builder.setInsertionPointAfter(newApplyop);
        // Get operands that defined by another AffineApplyOp
        for (auto operand : applyop.getOperands()){
          auto definedOp = operand.getDefiningOp();
          if(!definedOp)
            continue;
          if (auto nestapplyop = dyn_cast<AffineApplyOp>(definedOp)){
            if (llvm::find(nestedOperands, operand) == nestedOperands.end()){
              nestedOperands.push_back(operand);
              auto it = llvm::find(applyops, nestapplyop);
              if (it != applyops.end()) {
                unsigned index = std::distance(applyops.begin(), it);
                ApplyOpIndex.push_back(index);
              }
            }
          }
        }
      }
      
      // Create local buffer offset for dma & IOPush
      SmallVector<Value, 4> localOffsets;
      SmallVector<Value, 4> IOPushOffsets;
      for (unsigned i = 0; i < rank; i++){
        auto sizeInt = sizesInt[i];
        auto d0 = builder.getAffineDimExpr(0);
        auto map = AffineMap::get(1, 0, d0 * sizeInt, builder.getContext());
        // Add ApplyOp for dma
        auto loop = loops[i];
        auto var = loop.getInductionVar();
        auto newApplyopDMA = builder.create<AffineApplyOp>(loc, map, var);
        localOffsets.push_back(newApplyopDMA);
        // Add ApplyOp for IOPush
        builder.setInsertionPointToStart(loopBody);
        loop = loops[i];
        var = loop.getInductionVar();
        auto newApplyop = builder.create<AffineApplyOp>(loc, map, var);
        IOPushOffsets.push_back(newApplyop);
        builder.setInsertionPointAfter(newApplyopDMA);
      }
      // Create dma op to load data from external mem to L2 buffer
      builder.create<DmaOp>(loc, src,      offsets,      sizes,      strides,
                             allocOp, localOffsets, localSizes, localStrides);

      // Replace IOPush: Send data from L2 buffer to L1 buffer
      builder.setInsertionPoint(op);
      builder.create<IOPushOp>(loc, allocOp, IOPushOffsets, localSizes, 
                               localStrides, op.getDst());
      op.erase();

      // Update the loop variable used in AffineApply in the newInnerLoop
      auto numVi = newLoops.size();
      auto newInnerLoop = newLoops[newLoops.size()-1];
      for (unsigned i = 0; i < numVi; ++i) {
        auto oldvi = loops[i].getInductionVar();
        auto newvi = newLoops[i].getInductionVar();
        oldvi.replaceUsesWithIf(newvi,[&](OpOperand &use){
            return newInnerLoop->isProperAncestor(use.getOwner());
        });
      }

      // Update the Operands defined by AffineApply in the newInnerLoop
      for (unsigned i = 0; i < nestedOperands.size(); i++) {
        auto oldNestedOperand = nestedOperands[i];
        auto newNestedOperand = newApplyops[ApplyOpIndex[i]];
        oldNestedOperand.replaceUsesWithIf(newNestedOperand,[&](OpOperand &use){
            return newInnerLoop->isProperAncestor(use.getOwner());
        });
      }

      // Update the offsets of dma in the newInnerLoop
      for (unsigned i = 0; i < rank; i++) {
        auto oldOffset = offsets[i];
        auto definedOp = oldOffset.getDefiningOp();
        if(!definedOp || dyn_cast<ConstantOp>(definedOp))
          continue;
        auto applyop = dyn_cast<AffineApplyOp>(definedOp);
        auto it = llvm::find(applyops, applyop);
        if (it == applyops.end()){
          llvm::outs() << "IOpushOp involves offset not"
                       << "defined by AffineApplyOp & ConstantOp\n";
          return WalkResult::interrupt();
        }
        unsigned index = std::distance(applyops.begin(), it);
        auto newOffset = newApplyops[index];
        oldOffset.replaceUsesWithIf(newOffset,[&](OpOperand &use){
            return newInnerLoop->isProperAncestor(use.getOwner());
        });
      }
      return WalkResult::advance();
    });

    // Tranverse the IOPopOp and allocate L2 buffers
    auto flag1 = plForOp.walk([&](IOPopOp op){
      auto dst = op.getDst();
      auto offsets = op.getDstOffsets();
      auto sizes = op.getDstSizes();
      auto strides = op.getDstStrides();
      SmallVector<int64_t, 4> sizesInt;
      unsigned rank = offsets.size();
      auto type = dyn_cast<MemRefType>(dst.getType());
      SmallVector<int64_t, 4> bufSizes;
      SmallVector<AffineForOp, 4> loops;
      SmallVector<AffineApplyOp, 4> applyops;
      // Get the buffer size of each dim
      unsigned dim = 0;
      for (auto offset: offsets){
        SmallVector<Value, 4> operands;
        resolveOffset(offset,operands,applyops);
        std::reverse(applyops.begin(), applyops.end());
        for (auto operand : operands){
          // Get the trip count of the corresponding point loop
          auto loop = getForInductionVarOwner(operand);
          if (loop && llvm::find(band, loop) != band.end()) {
            SmallVector<Value, 4> foroperands;
            AffineMap map;
            getTripCountMapAndOperands(loop, &map, &foroperands);
            auto tripCount = map.getSingleConstantResult();
            if (!tripCount){
              llvm::outs() << "Involve loops with non-consant trip count!\n";
              return WalkResult::interrupt();
            }
            auto size = sizes[dim];
            auto constantOp = dyn_cast<arith::ConstantOp>(size.getDefiningOp());
            if(!constantOp)
              return WalkResult::interrupt();
            auto intAttr = dyn_cast<IntegerAttr>(constantOp.getValue());
            auto sizeInt = intAttr.getInt();
            sizesInt.push_back(sizeInt);
            bufSizes.push_back(tripCount*sizeInt);
            loops.push_back(loop);
            break;
          }
        }
        dim++;
      }

      // Allocate L2 buffer after the outer point loop
      builder.setInsertionPointToStart(&plFunc.getBody().front());
      auto allocOp 
           = builder.create<AllocOp>(loc, MemRefType::get(bufSizes,
                                     type.getElementType(), AffineMap(),
                                     (int)mlir::aries::adf::MemorySpace::L2));
      
      // Create static size and strides for dma & IOPush at the front of func
      builder.setInsertionPointToStart(&plFunc.getBody().front());
      auto indexType = builder.getIndexType();
      auto oneAttr = builder.getIntegerAttr(indexType, 1);
      auto oneValue = builder.create<arith::ConstantOp>(loc,indexType,oneAttr);
      builder.setInsertionPointAfter(oneValue);
      SmallVector<Value> localSizes;
      SmallVector<Value> localStrides;
      for(unsigned i=0; i< rank; i++){
        auto sizeAttr = builder.getIntegerAttr(indexType, sizesInt[i]);
        auto sizeValue 
             = builder.create<arith::ConstantOp>(loc, indexType, sizeAttr);
        builder.setInsertionPointAfter(sizeValue);
        localSizes.push_back(sizeValue);
        localStrides.push_back(oneValue);
      }

      // Load data from external mem to L2 buffer
      SmallVector<AffineForOp, 4> newLoops;
      builder.setInsertionPointAfter(outerloop);
      // Clone buffer related loops
      for (auto loop : loops){
        auto newForOp 
             = builder.create<AffineForOp>(loc,
                                           loop.getLowerBoundOperands(),
                                           loop.getLowerBoundMap(),
                                           loop.getUpperBoundOperands(),
                                           loop.getUpperBoundMap());
        newLoops.push_back(newForOp);
        builder.setInsertionPointToStart(newForOp.getBody());
      }
      // Create affine ApplyOps inside the innermost new loop
      SmallVector<AffineApplyOp, 4> newApplyops;
      SmallVector<Value, 4> nestedOperands;
      SmallVector<unsigned, 4> ApplyOpIndex;
      for (auto applyop : applyops){
        auto newApplyop = builder.create<AffineApplyOp>(loc, 
                                                        applyop.getAffineMap(),
                                                        applyop.getOperands());
        newApplyops.push_back(newApplyop);
        builder.setInsertionPointAfter(newApplyop);
        // Get operands that defined by another AffineApplyOp
        for (auto operand : applyop.getOperands()){
          auto definedOp = operand.getDefiningOp();
          if(!definedOp)
            continue;
          if (auto nestapplyop = dyn_cast<AffineApplyOp>(definedOp)){
            if (llvm::find(nestedOperands, operand) == nestedOperands.end()){
              nestedOperands.push_back(operand);
              auto it = llvm::find(applyops, nestapplyop);
              if (it != applyops.end()) {
                unsigned index = std::distance(applyops.begin(), it);
                ApplyOpIndex.push_back(index);
              }
            }
          }
        }
      }
      
      // Create local buffer offset for dma & IOPush
      SmallVector<Value, 4> localOffsets;
      SmallVector<Value, 4> IOPushOffsets;
      for (unsigned i = 0; i < rank; i++){
        auto sizeInt = sizesInt[i];
        auto d0 = builder.getAffineDimExpr(0);
        auto map = AffineMap::get(1, 0, d0 * sizeInt, builder.getContext());
        // Add ApplyOp for dma
        auto loop = loops[i];
        auto var = loop.getInductionVar();
        auto newApplyopDMA = builder.create<AffineApplyOp>(loc, map, var);
        localOffsets.push_back(newApplyopDMA);
        // Add ApplyOp for IOPush
        builder.setInsertionPointToStart(loopBody);
        loop = loops[i];
        var = loop.getInductionVar();
        auto newApplyop = builder.create<AffineApplyOp>(loc, map, var);
        IOPushOffsets.push_back(newApplyop);
        builder.setInsertionPointAfter(newApplyopDMA);
      }
      // Create dma op to load data from external mem to L2 buffer
      builder.create<DmaOp>(loc,allocOp, localOffsets, localSizes, localStrides,
                                dst,     offsets,      sizes,      strides);

      // Replace IOPush: Send data from L2 buffer to L1 buffer
      builder.setInsertionPoint(op);
      builder.create<IOPopOp>(loc, op.getSrc(), allocOp, 
                              IOPushOffsets, localSizes, localStrides);
      op.erase();

      // Update the loop variable used in AffineApply in the newInnerLoop
      auto numVi = newLoops.size();
      auto newInnerLoop = newLoops[newLoops.size()-1];
      for (unsigned i = 0; i < numVi; ++i) {
        auto oldvi = loops[i].getInductionVar();
        auto newvi = newLoops[i].getInductionVar();
        oldvi.replaceUsesWithIf(newvi,[&](OpOperand &use){
            return newInnerLoop->isProperAncestor(use.getOwner());
        });
      }

      // Update the Operands defined by AffineApply in the newInnerLoop
      for (unsigned i = 0; i < nestedOperands.size(); i++) {
        auto oldNestedOperand = nestedOperands[i];
        auto newNestedOperand = newApplyops[ApplyOpIndex[i]];
        oldNestedOperand.replaceUsesWithIf(newNestedOperand,[&](OpOperand &use){
            return newInnerLoop->isProperAncestor(use.getOwner());
        });
      }

      // Update the offsets of dma in the newInnerLoop
      for (unsigned i = 0; i < rank; i++) {
        auto oldOffset = offsets[i];
        auto definedOp = oldOffset.getDefiningOp();
        if(!definedOp || dyn_cast<ConstantOp>(definedOp))
          continue;
        auto applyop = dyn_cast<AffineApplyOp>(definedOp);
        auto it = llvm::find(applyops, applyop);
        if (it == applyops.end()){
          llvm::outs() << "IOpushOp involves offset not"
                       << "defined by AffineApplyOp & ConstantOp\n";
          return WalkResult::interrupt();
        }
        unsigned index = std::distance(applyops.begin(), it);
        auto newOffset = newApplyops[index];
        oldOffset.replaceUsesWithIf(newOffset,[&](OpOperand &use){
            return newInnerLoop->isProperAncestor(use.getOwner());
        });
      }
      return WalkResult::advance();
    });

    if (flag == WalkResult::interrupt() || flag1 == WalkResult::interrupt()) 
      return false;
      
    return true;
  }

  // Move the collected adf.cell to adf.cell.launch
  void UpdateCellLaunch(OpBuilder builder, LauchCellOp lauchcell, 
                        SmallVectorImpl<CallOp>& calls){
    auto &entryBlock = lauchcell.getBody().front();
    builder.setInsertionPointToStart(&entryBlock);
    for(auto call: calls)
      builder.insert(call);
  }

  bool IOMaterialize (ModuleOp mod, StringRef topFuncName) {
    bool flag = true;
    auto builder = OpBuilder(mod);
    FuncOp topFunc;
    if(!topFind(mod, topFunc, topFuncName)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }
    
    // Find the LauchCellOp
    // TODO: Handle Multiple LauchCellOps
    LauchCellOp lauchcell;
    topFunc.walk([&](LauchCellOp op){
      lauchcell = op;
    });
    if(!lauchcell)
      return true;

    // Materialize Push/Pop of GMIO
    auto boolPLIO = topFunc->getAttr("plio");
    SmallVector<CallOp> calls;
    SmallVector<Value> inputs;
    FuncOp plFunc;
    if(dyn_cast<BoolAttr>(boolPLIO).getValue()){
      Preprocess(lauchcell, calls);
      ArguDetect(lauchcell, inputs);
      PLFuncCreation(builder, topFunc, plFunc, lauchcell, inputs);
      flag = ADFPLAlloc(builder, plFunc);
      UpdateCellLaunch(builder, lauchcell, calls);
    }

    return flag;
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesPLIOMaterializePass() {
  return std::make_unique<AriesPLIOMaterialize>();
}

} // namespace aries
} // namespace mlir