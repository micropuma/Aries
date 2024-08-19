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
    SmallVector<AffineForOp, 4> loops;
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
    for (auto loop : loops){
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
    for (auto loop : loops){
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
      auto newApplyop = builder.create<AffineApplyOp>(loc, 
                                                      applyop.getAffineMap(),
                                                      applyop.getOperands());
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
      auto loop = loops[i];
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
      builder.create<IOPopOp>(loc, src, allocOp, 
                              IOOffsets, localSizes, localStrides);
    }

    // Update the loop variable used in AffineApply in the newInnerDMALoop
    auto numVi = newDMALoops.size();
    for (unsigned i = 0; i < numVi; ++i) {
      auto oldvi = loops[i].getInductionVar();
      auto newvi = newDMALoops[i].getInductionVar();
      oldvi.replaceUsesWithIf(newvi,[&](OpOperand &use){
          return newInnerDMALoop->isProperAncestor(use.getOwner());
      });
    }

    // Update the loop variable used in AffineApply in the newInnerIOLoop
    numVi = newIOLoops.size();
    for (unsigned i = 0; i < numVi; ++i) {
      auto oldvi = loops[i].getInductionVar();
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
      builder.create<AffineStoreOp>(loc, result, dst, newApplyopIOs);
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
      applyOperands.push_back(offset);
      applyOperands.push_back(var);
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
      applyOperands.push_back(offset);
      applyOperands.push_back(var);
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

  // This function works coalesce with ConvertDMAToAffine as it assumes the way
  // of constructing newApplyopL2DMA
  void L2BufferProcess(OpBuilder builder, FuncOp plFunc, 
                       SmallVector<AffineForOp, 6> band){
    auto loc = builder.getUnknownLoc();
    auto outerLoop = band[0];
    auto innerLoop = band[band.size()-1];
    auto innerYiled = innerLoop.getBody()->getTerminator();
    SmallVector<OpFoldResult> sizes;
    sizes.push_back(builder.getIndexAttr(1));
    auto streamAttr = builder.getStringAttr("stream");
    if(outerLoop->hasAttr("load")){
      for (auto storeOp: innerLoop.getOps<AffineStoreOp>()){
        auto memref = storeOp.getMemRef();
        auto val = storeOp.getValueToStore();
        auto type = dyn_cast<MemRefType>(memref.getType());
        auto elementType = type.getElementType();
        if (type.getMemorySpaceAsInt() != (int)MemorySpace::L2)
          return;
        auto rank = type.getRank();
        auto indices = storeOp.getIndices();
        SmallVector<AffineApplyOp, 4> applyops;
        SmallVector<AffineForOp, 4> loops;
        SmallVector<AffineForOp, 4> newForOps;
        builder.setInsertionPointAfter(outerLoop);
        for (unsigned i = 0; i < rank; i++){
          SmallVector<Value, 4> operands;
          auto index = indices[i];
          resolveOffset(index,operands,applyops);
          // create new for loops
          for (auto operand : operands){
            auto loop = getForInductionVarOwner(operand);
            if (loop && llvm::find(band, loop) != band.end()) {
              auto newForOp 
                   = builder.create<AffineForOp>(loc,
                                                 loop.getLowerBoundOperands(),
                                                 loop.getLowerBoundMap(),
                                                 loop.getUpperBoundOperands(),
                                                 loop.getUpperBoundMap());
              newForOps.push_back(newForOp);
              loops.push_back(loop);
              builder.setInsertionPointToStart(newForOp.getBody());
            }
          }
        }
        // Create affine store to memref with 1 element, then load data from the
        // single-element memref to L2 buffer. This is to represent FIFO in PL.
        builder.setInsertionPointToStart(&plFunc.getBody().front());
        auto allocOp 
             = builder.create<AllocOp>(loc, sizes, elementType, streamAttr);
        auto indexType = builder.getIndexType();
        auto zeroAttr = builder.getIntegerAttr(indexType, 0);
        auto zeroValue = builder.create<arith::ConstantOp>(loc, zeroAttr);
        SmallVector<Value> zeroValues(1, zeroValue);
        builder.setInsertionPoint(innerYiled);
        builder.create<AffineStoreOp>(loc, val, allocOp, zeroValues);
        storeOp.erase();
        auto innerNewLoop = newForOps[newForOps.size()-1];
        auto innerNewYiled = innerNewLoop.getBody()->getTerminator();
        builder.setInsertionPoint(innerNewYiled);
        auto newLoad = builder.create<AffineLoadOp>(loc, allocOp, zeroValues);
        builder.create<AffineStoreOp>(loc, newLoad, memref, indices);
        // Update the loop variables
        auto numVi = newForOps.size();
        for (unsigned i = 0; i < numVi; ++i) {
          auto oldvi = loops[i].getInductionVar();
          auto newvi = newForOps[i].getInductionVar();
          oldvi.replaceUsesWithIf(newvi,[&](OpOperand &use){
              return innerNewLoop->isProperAncestor(use.getOwner());
          });
        }
      }
    }else if(outerLoop->hasAttr("store")){
      for (auto loadOp: innerLoop.getOps<AffineLoadOp>()){
        
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
      SmallVector<AffineForOp, 6> band;
      getNestedLoops(band, forOp);
      AffineApplyPerfectize(band);
      //L2BufferProcess(builder, plFunc, band);
    }
      
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
    LauchCellOp lauchcell = getFirstOpOfType<LauchCellOp>(topFunc.getBody());
    if(!lauchcell)
      return true;

    // Materialize Push/Pop of PLIO
    auto boolPLIO = topFunc->getAttr("plio");
    SmallVector<CallOp> calls;
    SmallVector<Value> inputs;
    FuncOp plFunc;
    if(dyn_cast<BoolAttr>(boolPLIO).getValue()){
      Preprocess(lauchcell, calls);
      ArguDetect(lauchcell, inputs);
      PLFuncCreation(builder, topFunc, plFunc, lauchcell, inputs);
      flag = PLDataMovement(builder, plFunc);
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