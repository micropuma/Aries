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

  // Create PL func.func and pl.launch. Create Callop inside pl.launch
  void PLFuncCreation(OpBuilder builder, FuncOp topFunc, FuncOp& plFunc,
                      LauchCellOp lauchcell){
    auto loc = builder.getUnknownLoc();
    SmallVector<Value> inputs;
    auto liveness = Liveness(lauchcell);
    for (auto livein: liveness.getLiveIn(&lauchcell.getBody().front()))
      if (!lauchcell->isProperAncestor(livein.getParentBlock()->getParentOp()))
        inputs.push_back(livein);

    // Define the dma function with the detected inputs as arguments
    builder.setInsertionPoint(topFunc);
    auto funcName = "dma_pl";
    auto funcType = builder.getFunctionType(ValueRange(inputs), TypeRange({}));
    plFunc = builder.create<FuncOp>(builder.getUnknownLoc(),funcName,funcType);
    plFunc->setAttr("adf.pl",builder.getBoolAttr(true));
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
  
  // This is a specific function that perfectize the affine.apply ops between
  // nested for loops. May need to be extend to a general pass.
  void AffineApplyPerfectize(SmallVector<AffineForOp, 6>& band){
    // Move AffineApplyOp to the innerLoop
    auto outerLoop = band[0];
    auto innerLoop = band[band.size()-1];
    auto loopBody = innerLoop.getBody();
    SmallVector<AffineApplyOp, 6> applyOpsBefore;
    outerLoop.walk([&](AffineApplyOp op){
      applyOpsBefore.push_back(op);
    });
    std::reverse(applyOpsBefore.begin(), applyOpsBefore.end());
    for (auto applyOp :  applyOpsBefore)
      applyOp->moveBefore(&loopBody->front());
  }

  bool loopNormalize(AffineForOp plForOp, SmallVector<AffineForOp, 6>& band){
    // Get point loops in band
    getNestedLoopBand(plForOp.getRegion(),band);

    // Normalize point loops
    for (auto forOp : band){
      if(failed(normalizeAffineFor(forOp)))
        return false;
    }
    AffineApplyPerfectize(band);
    return true;
  }

  // This is a helper function that resolves the offset defined recursively by
  // nested AffineApplyOps 0) val: should be the offset of IOPush/IOPop
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
      applyops.push_back(applyop);
      for (auto operand : applyop.getOperands())
        resolveOffset(operand, operands, applyops);
    }else{
      operands.push_back(val);
      return;
    }
  }

  // Allocate L2 memory and add data movement involving L2 mem(PL buffer)
  // including load/store data from/to L3 mem (external)
  // send/receive data to/from L1 mem (AIE local)
  WalkResult IOProcesses(OpBuilder builder, FuncOp plFunc, Operation* op, 
                         SmallVector<AffineForOp, 6> band, unsigned& loadIndex,
                         unsigned& storeIndex, bool iopush){
    auto loc = builder.getUnknownLoc();
    IOPushOp iopushOp;
    IOPopOp iopopOp;
    Value src, dst;
    SmallVector<Value> offsets;
    SmallVector<Value> sizes;
    SmallVector<Value> strides;
    MemRefType type;
    if(iopush){
      iopushOp = dyn_cast<IOPushOp>(op);
      src = iopushOp.getSrc();
      type = dyn_cast<MemRefType>(src.getType());
      dst = iopushOp.getDst();
      offsets = iopushOp.getSrcOffsets();
      sizes = iopushOp.getSrcSizes();
      strides = iopushOp.getSrcStrides();
    }else{
      iopopOp = dyn_cast<IOPopOp>(op);
      src = iopopOp.getSrc();
      dst = iopopOp.getDst();
      type = dyn_cast<MemRefType>(dst.getType());
      offsets = iopopOp.getDstOffsets();
      sizes = iopopOp.getDstSizes();
      strides = iopopOp.getDstStrides();
    }

    //////////////////////////TODO////////////////////////
    /*
    Need to handle more general cases of the offest in IOPushOp/IOPopOp
    Now assumes this is a rectangular tiling. 
    the size of local buffer = point loop bounds * size of IOPush/IOPop src
    the offset of the local buffer = point loop variable * size of ~ src
    the stride of the local buffer = 1 
    */
    //////////////////////////////////////////////////////
    auto outerloop = band[0];
    SmallVector<int64_t, 4> sizesInt;
    unsigned rank = offsets.size();
    SmallVector<int64_t, 4> bufSizes;
    SmallVector<unsigned, 4> loopIndices;
    SmallVector<AffineApplyOp, 4> applyops;
    // Get the buffer size of each dim
    unsigned dim = 0;
    for (auto offset: offsets){
      SmallVector<Value, 4> operands;
      SmallVector<AffineApplyOp, 4> localApplyops;
      resolveOffset(offset,operands,applyops);
      for (auto operand : operands){
        // Get the trip count of the corresponding point loop
        auto loop = getForInductionVarOwner(operand);
        if(!loop)
          continue;
        auto it = llvm::find(band, loop);
        if (it != band.end()) {
          unsigned index = std::distance(band.begin(), it);
          loopIndices.push_back(index);
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
    
    // Create static size and strides for dma & IOPush/IOPop
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
    // Create loops for DMA Ops
    SmallVector<AffineForOp, 4> newDMALoops;
    if (iopush)
      builder.setInsertionPoint(outerloop);
    else
      builder.setInsertionPointAfter(outerloop);
    auto orderedIndices = loopIndices;
    llvm::sort(orderedIndices);
    for (auto loopIndex : orderedIndices){
      auto loop = band[loopIndex];
      auto newDMAForOp
           = builder.create<AffineForOp>(loc,
                                         loop.getLowerBoundOperands(),
                                         loop.getLowerBoundMap(),
                                         loop.getUpperBoundOperands(),
                                         loop.getUpperBoundMap());
      newDMALoops.push_back(newDMAForOp);
      builder.setInsertionPointToStart(newDMAForOp.getBody());
    }
    // Create loops for IOPush/IOPop Ops
    SmallVector<AffineForOp, 4> newIOLoops;
    if (iopush)
      builder.setInsertionPoint(outerloop);
    else
      builder.setInsertionPointAfter(outerloop);
    for (auto loop : band){
      auto newIOForOp 
           = builder.create<AffineForOp>(loc,
                                         loop.getLowerBoundOperands(),
                                         loop.getLowerBoundMap(),
                                         loop.getUpperBoundOperands(),
                                         loop.getUpperBoundMap());
      newIOLoops.push_back(newIOForOp);
      builder.setInsertionPointToStart(newIOForOp.getBody());
    }
    // Anotate the outer new point loops with name and index
    auto newDMAOuterloop = newDMALoops[0];
    auto newIOOuterloop = newIOLoops[0];
    auto loadAttr = builder.getIntegerAttr(builder.getIndexType(),loadIndex);
    auto storeAttr = builder.getIntegerAttr(builder.getIndexType(),storeIndex);
    if(iopush){
      newDMAOuterloop->setAttr("load",loadAttr);
      newIOOuterloop->setAttr("send",loadAttr);
      loadIndex++;
    }else{
      newDMAOuterloop->setAttr("store",storeAttr);
      newIOOuterloop->setAttr("receive",storeAttr);
      storeIndex++;
    }
    // Create affine ApplyOps inside the innermost new loop
    auto newInnerDMALoop = newDMALoops[newDMALoops.size()-1];
    auto newInnerIOLoop = newIOLoops[newIOLoops.size()-1];
    auto newInnerDMAYiled = newInnerDMALoop.getBody()->getTerminator();
    auto newInnerIOYiled = newInnerIOLoop.getBody()->getTerminator();
    std::reverse(applyops.begin(), applyops.end());
    SmallVector<AffineApplyOp, 4> newApplyops;
    SmallVector<Value, 4> nestedOperands;
    SmallVector<unsigned, 4> ApplyOpIndex;
    for (auto applyop : applyops){
      builder.setInsertionPoint(newInnerDMAYiled);
      auto *clonedOp = applyop->clone();
      builder.insert(clonedOp);
      auto newApplyop = dyn_cast<AffineApplyOp>(clonedOp);
      newApplyops.push_back(newApplyop);
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
    // Create local buffer offset for dma & IOPush/IOPop
    SmallVector<Value, 4> localOffsets;
    SmallVector<Value, 4> IOOffsets;
    for (unsigned i = 0; i < rank; i++){
      auto sizeInt = sizesInt[i];
      auto d0 = builder.getAffineDimExpr(0);
      auto map = AffineMap::get(1, 0, d0 * sizeInt, builder.getContext());
      // Add ApplyOp for dma
      auto loopIndex = loopIndices[i];
      auto loop = band[loopIndex];
      auto var = loop.getInductionVar();
      builder.setInsertionPoint(newInnerDMAYiled);
      auto newApplyopDMA = builder.create<AffineApplyOp>(loc, map, var);
      localOffsets.push_back(newApplyopDMA);
      // Add ApplyOp for IOPush/IOPop
      builder.setInsertionPoint(newInnerIOYiled);
      auto newApplyopIO = builder.create<AffineApplyOp>(loc, map, var);
      IOOffsets.push_back(newApplyopIO);
    }
    if (iopush){
      // Create dma op to load data from external mem to L2 buffer
      // Replace IOPush: Send data from L2 buffer to L1 buffer
      builder.setInsertionPoint(newInnerDMAYiled);
      builder.create<DmaOp>(loc, src,      offsets,      sizes,      strides,
                             allocOp, localOffsets, localSizes, localStrides);
      builder.setInsertionPoint(newInnerIOYiled);
      builder.create<IOPushOp>(loc, allocOp, IOOffsets, localSizes, 
                               localStrides, dst);
    }else{
      // Create dma op to store data from L2 buffer to external mem
      // Replace IOPop: Receive data from L1 buffer to L2 buffer
      builder.setInsertionPoint(newInnerDMAYiled);
      builder.create<DmaOp>(loc,allocOp, localOffsets, localSizes, localStrides,
                                dst,     offsets,      sizes,      strides);
      builder.setInsertionPoint(newInnerIOYiled);
      auto popOp = builder.create<IOPopOp>(loc, src, allocOp, 
                                           IOOffsets, localSizes, localStrides);
      // check if there is output buffer reuse and set an Attr
      if (loopIndices.size()!=band.size())
        popOp->setAttr("reduction", builder.getUnitAttr());
    }

    // Update the loop variable used in AffineApply in the newInnerDMALoop
    auto numVi = newDMALoops.size();
    for (unsigned i = 0; i < numVi; ++i) {
      auto oldloopIndex = orderedIndices[i];
      auto oldvi = band[oldloopIndex].getInductionVar();
      auto newvi = newDMALoops[i].getInductionVar();
      oldvi.replaceUsesWithIf(newvi,[&](OpOperand &use){
          return newInnerDMALoop->isProperAncestor(use.getOwner());
      });
    }

    // Update the loop variable used in AffineApply in the newInnerIOLoop
    numVi = newIOLoops.size();
    for (unsigned i = 0; i < numVi; ++i) {
      auto oldvi = band[i].getInductionVar();
      auto newvi = newIOLoops[i].getInductionVar();
      oldvi.replaceUsesWithIf(newvi,[&](OpOperand &use){
          return newInnerIOLoop->isProperAncestor(use.getOwner());
      });
    }

    // Update the Operands defined by AffineApply in the newInnerDMALoop
    for (unsigned i = 0; i < nestedOperands.size(); i++) {
      auto oldNestedOperand = nestedOperands[i];
      auto newNestedOperand = newApplyops[ApplyOpIndex[i]];
      oldNestedOperand.replaceUsesWithIf(newNestedOperand,[&](OpOperand &use){
          return newInnerDMALoop->isProperAncestor(use.getOwner());
      });
    }

    // Update the offsets of dma in the newInnerDMALoop
    for (unsigned i = 0; i < rank; i++) {
      auto oldOffset = offsets[i];
      auto definedOp = oldOffset.getDefiningOp();
      if(!definedOp || dyn_cast<ConstantOp>(definedOp))
        continue;
      auto applyop = dyn_cast<AffineApplyOp>(definedOp);
      auto it = llvm::find(applyops, applyop);
      if (it == applyops.end()){
        llvm::outs() << "IOpushOp/IOpopOp involves offset not"
                     << "defined by AffineApplyOp & ConstantOp\n";
        return WalkResult::interrupt();
      }
      unsigned index = std::distance(applyops.begin(), it);
      auto newOffset = newApplyops[index];
      oldOffset.replaceUsesWithIf(newOffset,[&](OpOperand &use){
          return newInnerDMALoop->isProperAncestor(use.getOwner());
      });
    }
    return WalkResult::advance();
  }

  WalkResult ConvertIOToAffine(OpBuilder builder, Operation* op,  bool iopush){
    auto loc = builder.getUnknownLoc();
    IOPushOp iopushOp;
    IOPopOp iopopOp;
    Value src, dst;
    SmallVector<Value> offsets;
    SmallVector<Value> sizes;
    MemRefType type;
    if(iopush){
      iopushOp = dyn_cast<IOPushOp>(op);
      src = iopushOp.getSrc();
      type = dyn_cast<MemRefType>(src.getType());
      dst = iopushOp.getDst();
      offsets = iopushOp.getSrcOffsets();
      sizes = iopushOp.getSrcSizes();
      builder.setInsertionPoint(iopushOp);
    }else{
      iopopOp = dyn_cast<IOPopOp>(op);
      src = iopopOp.getSrc();
      dst = iopopOp.getDst();
      type = dyn_cast<MemRefType>(dst.getType());
      offsets = iopopOp.getDstOffsets();
      sizes = iopopOp.getDstSizes();
      builder.setInsertionPoint(iopopOp);
    } // Create for loops for IOPush/Pop Ops
    SmallVector<AffineForOp, 4> newIOLoops;
    auto rank = offsets.size();
    for(unsigned i = 0; i < rank; i++){
      auto size = sizes[i];
      auto constantSize = dyn_cast<arith::ConstantOp>(size.getDefiningOp());
      if(!constantSize)
        return WalkResult::interrupt();
      auto sizeAttr = dyn_cast<IntegerAttr>(constantSize.getValue());
      auto sizeInt = sizeAttr.getInt();
      auto newIOForOp 
           = builder.create<AffineForOp>(loc, 0, sizeInt, 1);
      newIOLoops.push_back(newIOForOp);
      builder.setInsertionPointToStart(newIOForOp.getBody());
    }// Get the access operands of affine load/store
    auto newInnerIOLoop = newIOLoops[newIOLoops.size()-1];
    auto newInnerIOYiled = newInnerIOLoop.getBody()->getTerminator();
    auto d0 = builder.getAffineDimExpr(0);
    auto d1 = builder.getAffineDimExpr(1);
    auto map = AffineMap::get(2, 0, d0 + d1, builder.getContext());
    SmallVector<Value> newApplyopIOs;
    for(unsigned i = 0; i < rank; i++){
      SmallVector<Value, 2> applyOperands;
      auto newIOLoop = newIOLoops[i];
      auto var = newIOLoop.getInductionVar();
      auto offset = offsets[i];
      applyOperands.push_back(var);
      applyOperands.push_back(offset);
      builder.setInsertionPoint(newInnerIOYiled);
      auto newApplyopIO=builder.create<AffineApplyOp>(loc, map, applyOperands);
      newApplyopIOs.push_back(newApplyopIO);
    } // Replace IOPush/Pop Ops with affine.load and affine.store
    if(iopush){
      auto loadOp = builder.create<AffineLoadOp>(loc, src, newApplyopIOs);
      builder.create<IOWriteOp>(loc, loadOp, dst);
      iopushOp.erase();
    }else{
      auto result = builder.create<IOReadOp>(loc, type.getElementType(), src);
      if(iopopOp->hasAttr("reduction")){
        auto loadOp = builder.create<AffineLoadOp>(loc, dst, newApplyopIOs);
        Value addOp;
        if (type.getElementType().isa<FloatType>())
          addOp = builder.create<arith::AddFOp>(loc, loadOp, result);
        else
          addOp = builder.create<arith::AddIOp>(loc, loadOp, result);
        builder.create<AffineStoreOp>(loc, addOp, dst, newApplyopIOs);
      }else{
        builder.create<AffineStoreOp>(loc, result, dst, newApplyopIOs);
      }
      iopopOp.erase();
    }
    return WalkResult::advance();
  }

  WalkResult ConvertDMAToAffine(OpBuilder builder, Operation* op){
    auto loc = builder.getUnknownLoc();
    auto context = builder.getContext();
    auto dmaOp = dyn_cast<DmaOp>(op);
    auto src = dmaOp.getSrc();
    auto srcType = dyn_cast<MemRefType>(src.getType());
    auto dst = dmaOp.getDst();
    auto dstType = dyn_cast<MemRefType>(dst.getType());
    SmallVector<Value> L2Sizes;
    SmallVector<Value> L2Offsets;
    SmallVector<Value> L3Offsets;
    SmallVector<Value> L3Strides;
    bool dmaLoad;
    if (srcType.getMemorySpaceAsInt() == (int)MemorySpace::L2){
      L2Sizes   = dmaOp.getSrcSizes();
      L2Offsets = dmaOp.getSrcOffsets();
      L3Offsets = dmaOp.getDstOffsets();
      L3Strides = dmaOp.getDstStrides();
      dmaLoad = false;
    }else if(dstType.getMemorySpaceAsInt() == (int)MemorySpace::L2){
      L2Sizes   = dmaOp.getDstSizes();
      L2Offsets = dmaOp.getDstOffsets();
      L3Offsets = dmaOp.getSrcOffsets();
      L3Strides = dmaOp.getSrcStrides();
      dmaLoad = true;
    }else{
      return WalkResult::interrupt();
    }
    // Create for loops for DMA Ops
    builder.setInsertionPoint(dmaOp);
    SmallVector<AffineForOp, 4> newDMALoops;
    auto rank = L2Sizes.size();
    for(unsigned i = 0; i < rank; i++){
      Value size = L2Sizes[i];
      auto constantSize = dyn_cast<arith::ConstantOp>(size.getDefiningOp());
      if(!constantSize)
        return WalkResult::interrupt();
      auto sizeAttr = dyn_cast<IntegerAttr>(constantSize.getValue());
      auto sizeInt = sizeAttr.getInt();
      auto newDMAForOp 
           = builder.create<AffineForOp>(loc, 0, sizeInt, 1);
      newDMALoops.push_back(newDMAForOp);
      builder.setInsertionPointToStart(newDMAForOp.getBody());
    }
    auto newInnerDMALoop = newDMALoops[newDMALoops.size()-1];
    auto newInnerDMAYiled = newInnerDMALoop.getBody()->getTerminator();
    // Create mem access for L2 buffer
    SmallVector<Value> newApplyopL2DMAs;
    auto d0 = builder.getAffineDimExpr(0);
    auto d1 = builder.getAffineDimExpr(1);
    auto map = AffineMap::get(2, 0, d0 + d1, context);
    for(unsigned i = 0; i < rank; i++){
      SmallVector<Value, 2> applyOperands;
      auto newIOLoop = newDMALoops[i];
      auto var = newIOLoop.getInductionVar();
      auto offset = L2Offsets[i];
      applyOperands.push_back(var);
      applyOperands.push_back(offset);
      builder.setInsertionPoint(newInnerDMAYiled);
      auto newApplyopL2DMA 
           = builder.create<AffineApplyOp>(loc, map, applyOperands);
      newApplyopL2DMAs.push_back(newApplyopL2DMA);
    }
    // Create mem access for L3 buffer
    SmallVector<Value> newApplyopL3DMAs;
    for(unsigned i = 0; i < rank; i++){
      SmallVector<Value, 2> applyOperands;
      auto newIOLoop = newDMALoops[i];
      auto stride = L3Strides[i];
      auto constantStride = dyn_cast<arith::ConstantOp>(stride.getDefiningOp());
      if(!constantStride)
        return WalkResult::interrupt();
      auto strideAttr = dyn_cast<IntegerAttr>(constantStride.getValue());
      auto StrideInt = strideAttr.getInt();
      auto map = AffineMap::get(2, 0, d0 * StrideInt + d1, context);
      auto var = newIOLoop.getInductionVar();
      auto offset = L3Offsets[i];
      applyOperands.push_back(var);
      applyOperands.push_back(offset);
      builder.setInsertionPoint(newInnerDMAYiled);
      auto newApplyopL3DMA 
           = builder.create<AffineApplyOp>(loc, map, applyOperands);
      newApplyopL3DMAs.push_back(newApplyopL3DMA);
    }
    if(dmaLoad){
      auto loadOp = builder.create<AffineLoadOp>(loc, src, newApplyopL3DMAs);
      builder.create<AffineStoreOp>(loc, loadOp, dst, newApplyopL2DMAs);
    }else{
      auto loadOp = builder.create<AffineLoadOp>(loc, src, newApplyopL2DMAs);
      builder.create<AffineStoreOp>(loc, loadOp, dst, newApplyopL3DMAs); 
    }
    dmaOp.erase();
    return WalkResult::advance();
  }

  void L2BufferProcess(OpBuilder builder, FuncOp plFunc, AffineStoreOp storeOp,
                       AffineLoadOp loadOp, SmallVector<AffineForOp, 6> band, 
                       bool load){
    auto loc = builder.getUnknownLoc();
    auto outerLoop = band[0];
    auto innerLoop = band[band.size()-1];
    SmallVector<OpFoldResult> sizes;
    sizes.push_back(builder.getIndexAttr(1));
    auto streamAttr = builder.getStringAttr("stream");
    auto indexType = builder.getIndexType();
    auto zeroAttr = builder.getIntegerAttr(indexType, 0);
    Value memref, val;
    SmallVector<Value> indices;
    if(load){
      memref = storeOp.getMemRef();
      val = storeOp.getValueToStore();
      indices = storeOp.getIndices();
    }else{
      memref = loadOp.getMemRef();
      val = loadOp.getResult();
      indices = loadOp.getIndices();
    }
    auto type = dyn_cast<MemRefType>(memref.getType());
    auto elementType = type.getElementType();
    if (type.getMemorySpaceAsInt() != (int)MemorySpace::L2)
      return;
    // Alloc single-element memref to represent stream FIFO
    builder.setInsertionPointToStart(&plFunc.getBody().front());
    auto zeroValue = builder.create<arith::ConstantOp>(loc, zeroAttr);
    SmallVector<Value> zeroValues(1, zeroValue);
    auto allocOp 
         = builder.create<AllocOp>(loc, sizes, elementType, streamAttr);
    if(load){
      builder.setInsertionPoint(storeOp);
      builder.create<AffineStoreOp>(loc, val, allocOp, zeroValues);
      builder.setInsertionPointAfter(outerLoop);
    }else{
      builder.setInsertionPoint(loadOp);
      auto newLoadOp = builder.create<AffineLoadOp>(loc, allocOp, zeroValues);
      val.replaceUsesWithIf(newLoadOp,[&](OpOperand &use){
          return innerLoop->isProperAncestor(use.getOwner());
      });
      builder.setInsertionPoint(outerLoop);
    }
    // Create newForOps for reading data from single-element memref 
    // and store to L2 buffer. This is to represent FIFO in PL.
    SmallVector<AffineForOp, 4> newForOps;
    for (auto loop: band){
      auto newForOp 
           = builder.create<AffineForOp>(loc,
                                         loop.getLowerBoundOperands(),
                                         loop.getLowerBoundMap(),
                                         loop.getUpperBoundOperands(),
                                         loop.getUpperBoundMap(),
                                         loop.getStepAsInt());
      newForOps.push_back(newForOp);
      builder.setInsertionPointToStart(newForOp.getBody());
    }
    // Get the affine.apply ops used in affine.store/load op
    auto rank = type.getRank();
    SmallVector<AffineApplyOp, 4> applyops;
    for (unsigned i = 0; i < rank; i++){
      SmallVector<Value, 4> operands;
      auto index = indices[i];
      resolveOffset(index,operands,applyops);
    }
    // Clone and move the used affine apply ops inside newForOps
    auto outerNewLoop = newForOps[0];
    auto innerNewLoop = newForOps[newForOps.size()-1];
    auto innerNewYiled = innerNewLoop.getBody()->getTerminator();
    SmallVector<AffineApplyOp, 4> newApplyops;
    std::reverse(applyops.begin(), applyops.end());
    builder.setInsertionPoint(innerNewYiled);
    for(auto applyop : applyops){
      auto *clonedOp = applyop->clone();
      builder.insert(clonedOp);
      auto newApplyOp = dyn_cast<AffineApplyOp>(clonedOp);
      newApplyops.push_back(newApplyOp);
    }
    // Load data from single-element memref and store to L2 buffer
    if(load){
      auto newLoad = builder.create<AffineLoadOp>(loc, allocOp, zeroValues);
      builder.create<AffineStoreOp>(loc, newLoad, memref, indices);
      storeOp.erase();
      auto Attr = outerLoop->getAttr("load");
      outerNewLoop->setAttr("send", Attr);
    }else{
      auto newLoad = builder.create<AffineLoadOp>(loc, memref, indices);
      builder.create<AffineStoreOp>(loc, newLoad, allocOp, zeroValues);
      loadOp.erase();
      auto Attr = outerLoop->getAttr("store");
      outerNewLoop->setAttr("receive", Attr);
    }
    // Update the loop variables
    auto numVi = newForOps.size();
    for (unsigned i = 0; i < numVi; ++i) {
      auto oldvi = band[i].getInductionVar();
      auto newvi = newForOps[i].getInductionVar();
      oldvi.replaceUsesWithIf(newvi,[&](OpOperand &use){
          return innerNewLoop->isProperAncestor(use.getOwner());
      });
    }
    // Update the AffineApplyOp used in the innerNewLoop
    for (unsigned i = 0; i < applyops.size(); i++) {
      auto oldApplyOp = applyops[i].getResult();
      auto newApplyOp = newApplyops[i].getResult();
      oldApplyOp.replaceUsesWithIf(newApplyOp,[&](OpOperand &use){
          return innerNewLoop->isProperAncestor(use.getOwner());
      });
    }
  }

  // Split the loops marked by "load,store,send,receive" and then merge them
  // into the outmost tileBand by identify there attributes
  void PLLoopSplit(OpBuilder builder, FuncOp plFunc, AffineForOp plForOp){
    auto loc = builder.getUnknownLoc();
    SmallVector<AffineForOp, 6> tileBand;
    getNestedLoopBand(plFunc.getRegion(),tileBand);
    auto outerTileBand = tileBand[0];
    // Tranverse the forOps and group then by the attribute 
    DenseMap<StringRef, SmallVector<unsigned, 4>> groups;
    SmallVector<AffineForOp, 4> forOps;
    unsigned index = 0;
    for (auto forOp: llvm::make_early_inc_range(plForOp.getOps<AffineForOp>())){
      if(auto Attr = forOp->getAttrOfType<IntegerAttr>("load")){
        std::string str = "load" + std::to_string(Attr.getInt());
        StringRef strRef(str);
        groups[strRef].push_back(index++);
        forOps.push_back(forOp);
      }else if(auto Attr = forOp->getAttrOfType<IntegerAttr>("store")){
        std::string str = "store" + std::to_string(Attr.getInt());
        StringRef strRef(str);
        groups[strRef].push_back(index++);
        forOps.push_back(forOp);
      }else if(auto Attr = forOp->getAttrOfType<IntegerAttr>("send")){
        std::string str = "send" + std::to_string(Attr.getInt());
        StringRef strRef(str);
        groups[strRef].push_back(index++);
        forOps.push_back(forOp);
      }else if(auto Attr = forOp->getAttrOfType<IntegerAttr>("receive")){
        std::string str = "receive" + std::to_string(Attr.getInt());
        StringRef strRef(str);
        groups[strRef].push_back(index++);
        forOps.push_back(forOp);
      }
    }
    // Move the forOps with the same attribute to the same tileBand
    for (const auto &group : groups) {
      SmallVector<AffineForOp, 4> newForOps;
      builder.setInsertionPoint(outerTileBand);
      for (auto loop: tileBand){
        auto newForOp 
             = builder.create<AffineForOp>(loc,
                                           loop.getLowerBoundOperands(),
                                           loop.getLowerBoundMap(),
                                           loop.getUpperBoundOperands(),
                                           loop.getUpperBoundMap(),
                                           loop.getStepAsInt());
        newForOps.push_back(newForOp);
        builder.setInsertionPointToStart(newForOp.getBody());
      }
      auto outerNewloop = newForOps[0];
      auto innerNewLoop = newForOps[newForOps.size()-1];
      auto innerNewYiled = innerNewLoop.getBody()->getTerminator();
      unsigned cnt = 0;
      for (unsigned idx : group.second) {
        // Move the forOp to the new loop nests and set new Attrs
        auto forOp = forOps[idx];
        forOp->moveBefore(innerNewYiled);
        auto indexType = builder.getIndexType();
        auto newAttr = builder.getIntegerAttr(indexType, cnt++);
        if(auto Attr = forOp->getAttr("load")){
          outerNewloop->setAttr("load", Attr);
          forOp->removeAttr("load");
        }else if(auto Attr = forOp->getAttr("store")){
          outerNewloop->setAttr("store", Attr);
          forOp->removeAttr("store");
        }else if(auto Attr = forOp->getAttr("send")){
          outerNewloop->setAttr("send", Attr);
          forOp->removeAttr("send");
          forOp->setAttr("module", newAttr);
        }else if(auto Attr = forOp->getAttr("receive")){
          outerNewloop->setAttr("receive", Attr);
          forOp->removeAttr("receive");
          forOp->setAttr("module", newAttr);
        }
      }
      // Update the loop variables
      auto numVi = newForOps.size();
      for (unsigned i = 0; i < numVi; ++i) {
        auto oldvi = tileBand[i].getInductionVar();
        auto newvi = newForOps[i].getInductionVar();
        oldvi.replaceUsesWithIf(newvi,[&](OpOperand &use){
            return innerNewLoop->isProperAncestor(use.getOwner());
        });
      }
    }
    outerTileBand.erase();
  }

  // Tranverse all the forOps marked by "load,store,send,receive" and split
  // them into new functions marked by adf.pl
  void PLFuncSplit(OpBuilder builder, FuncOp plFunc){
    auto loc = builder.getUnknownLoc();
    for (auto forOp: llvm::make_early_inc_range(plFunc.getOps<AffineForOp>())){
      SmallVector<Operation*, 4> Ops;
      SmallVector<Value> inputs(forOp.getOperands());
      auto liveness = Liveness(forOp);
      for (auto livein: liveness.getLiveIn(forOp.getBody()))
        if (!forOp->isProperAncestor(livein.getParentBlock()->getParentOp())){
          auto definedOp = livein.getDefiningOp();
          if(definedOp){
            if(auto allocOp = dyn_cast<AllocOp>(definedOp)){
              auto type = dyn_cast<MemRefType>(livein.getType());
              if(auto memorySpace = type.getMemorySpace()){
                auto intAttr = dyn_cast<IntegerAttr>(memorySpace);
                if(intAttr && intAttr.getInt() == (int)MemorySpace::L2){
                  Ops.push_back(definedOp);
                  continue;
                }
              }
            }else if(auto constOp = dyn_cast<arith::ConstantOp>(definedOp)){
              Ops.push_back(definedOp);
              continue;
            }
          }
          inputs.push_back(livein);
        }
      
      builder.setInsertionPoint(plFunc);
      std::string funcName;
      if(auto Attr = forOp->getAttrOfType<IntegerAttr>("load")){
        funcName = "load" + std::to_string(Attr.getInt());
        forOp->removeAttr("load");
      }else if(auto Attr = forOp->getAttrOfType<IntegerAttr>("store")){
        funcName = "store" + std::to_string(Attr.getInt());
        forOp->removeAttr("store");
      }else if(auto Attr = forOp->getAttrOfType<IntegerAttr>("send")){
        funcName = "send" + std::to_string(Attr.getInt());
        forOp->removeAttr("send");
      }else if(auto Attr = forOp->getAttrOfType<IntegerAttr>("receive")){
        funcName = "receive" + std::to_string(Attr.getInt());
        forOp->removeAttr("receive");
      }else{
       continue; 
      }
      auto funcType = builder.getFunctionType(ValueRange(inputs),TypeRange({}));
      auto newfunc = builder.create<FuncOp>(
                                  builder.getUnknownLoc(), funcName, funcType);
      newfunc->setAttr("adf.pl",builder.getUnitAttr());
      auto destBlock = newfunc.addEntryBlock();
      builder.setInsertionPointToEnd(destBlock);
      auto returnOp = builder.create<ReturnOp>(builder.getUnknownLoc());

      // Move L2 buffer/Constant definition inside each function
      builder.setInsertionPointToStart(destBlock);
      SmallVector<Operation*, 4> newOps;
      for(auto *op : Ops){
        auto newOp = op->clone();
        builder.insert(newOp);
        newOps.push_back(newOp);
      }

      // Move the entire block of outerPointLoop before the returnOp
      builder.setInsertionPointToEnd(destBlock);
      forOp->moveBefore(returnOp);

      // Create the function CallOp in plFunc
      auto topReturnOp = plFunc.getBody().front().getTerminator();
      builder.setInsertionPoint(topReturnOp);
      builder.create<CallOp>(loc, newfunc, inputs);

      // Update the references in the newfunc after move
      auto numArg = destBlock->getNumArguments();
      for (unsigned i = 0; i < numArg; ++i) {
        auto sourceArg = inputs[i];
        auto destArg = destBlock->getArgument(i);
        sourceArg.replaceUsesWithIf(destArg,[&](OpOperand &use){
            return newfunc->isProperAncestor(use.getOwner());
        });
      }
      auto opSize = newOps.size();
      for (unsigned i = 0; i < opSize; ++i) {
        auto oldVal = Ops[i]->getResult(0);
        auto newVal = newOps[i]->getResult(0);
        oldVal.replaceUsesWithIf(newVal,[&](OpOperand &use){
            return newfunc->isProperAncestor(use.getOwner());
        });
      }
    }
  }

  // Allocate L2 memory & add data movement
  bool PLDataMovement(OpBuilder builder, FuncOp plFunc){
    AffineForOp plForOp;
    plFunc.walk([&](AffineForOp forOp){
      if(forOp->hasAttr("Array_Partition"))
        plForOp = forOp;
    });
    if(!plForOp){
      llvm::outs() << "Array_Partition loop not found\n";
      return false;
    }
    SmallVector<AffineForOp, 6> band;
    if(!loopNormalize(plForOp, band))
      return false;

    // Tranverse the IOPushOps/IOPopOps and allocate L2 buffers
    unsigned loadIndex = 0;
    unsigned storeIndex = 0;
    auto flag = plForOp.walk([&](Operation* op){
      WalkResult result;
      if(dyn_cast<IOPushOp>(op))
        result = IOProcesses(builder, plFunc, op, band, 
                             loadIndex, storeIndex, true);
      else if(dyn_cast<IOPopOp>(op))
        result = IOProcesses(builder, plFunc, op, band, 
                             loadIndex, storeIndex, false);
      else
        return WalkResult::advance();
      return result;
    });
    if (flag == WalkResult::interrupt()) 
      return false;

    // After Processes IOPush/IOPop in the outerloop, safe to erase outerLoop
    auto outerLoop = band[0];
    outerLoop.erase();

    // Tranverse the IOPushOps/IOPopOps and convert them to affine load/store
    flag = plForOp.walk([&](Operation* op){
      WalkResult result;
      if(dyn_cast<IOPushOp>(op))
        result = ConvertIOToAffine(builder, op, true);
      else if(dyn_cast<IOPopOp>(op))
        result = ConvertIOToAffine(builder, op, false);
      else if(dyn_cast<DmaOp>(op))
        result = ConvertDMAToAffine(builder, op);
      else
        return WalkResult::advance();
      return result;
    });
    if (flag == WalkResult::interrupt()) 
      return false;
    
    // Tranverse all the outer point dma loops(involve L3 mem) and change the
    // L2 buffer access with stream access
    for (AffineForOp forOp : plForOp.getOps<AffineForOp>()) {
      if(!forOp->hasAttr("load") && !forOp->hasAttr("store"))
        continue;
      SmallVector<AffineForOp, 6> band;
      getNestedLoops(band, forOp);
      AffineApplyPerfectize(band);
      auto outerLoop = band[0];
      auto innerLoop = band[band.size()-1];
      AffineStoreOp emptyStoreOp;
      AffineLoadOp emptyLoadOp;
      if(outerLoop->hasAttr("load"))
        for (auto storeOp: 
             llvm::make_early_inc_range(innerLoop.getOps<AffineStoreOp>()))
          L2BufferProcess(builder, plFunc, storeOp, emptyLoadOp, band, true);
      else
        for (auto loadOp: 
             llvm::make_early_inc_range(innerLoop.getOps<AffineLoadOp>()))
          L2BufferProcess(builder, plFunc, emptyStoreOp, loadOp, band, false);
    }

    // Post processes, move each loop marked by "load,store,receive,send" to the
    // outermost temporal tileBand 
    PLLoopSplit(builder, plFunc, plForOp);

    // Create each loop with a func marked by adf.pl
    PLFuncSplit(builder, plFunc);
      
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
    auto builder = OpBuilder(mod);
    FuncOp topFunc;
    if(!topFind(mod, topFunc, topFuncName)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }
    
    // Find the LauchCellOp
    // TODO: Handle Multiple LauchCellOps
    LauchCellOp lauchcell = getFirstOpOfType<LauchCellOp>(topFunc.getBody());
    if(!lauchcell)
      return true;

    // Materialize Push/Pop of PLIO
    auto boolPLIO = topFunc->getAttr("plio");
    SmallVector<CallOp> calls;
    FuncOp plFunc;
    if(dyn_cast<BoolAttr>(boolPLIO).getValue()){
      Preprocess(lauchcell, calls);
      PLFuncCreation(builder, topFunc, plFunc, lauchcell);
      if(!PLDataMovement(builder, plFunc))
        return false;
      UpdateCellLaunch(builder, lauchcell, calls);
    }
    return true;
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