#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "llvm/Support/Debug.h"
#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "aries/Dialect/ADF/ADFDialect.h"

using namespace mlir;
using namespace aries;
using namespace adf;
using namespace mlir::func;
using namespace mlir::affine;
using namespace mlir::memref;


namespace {

struct AriesDMAToIO : public AriesDMAToIOBase<AriesDMAToIO> {
public:
  AriesDMAToIO() = default;
  AriesDMAToIO(const AriesOptions &opts) {
    PortType=opts.OptPortType;
    PortWidth=opts.OptPortWidth;
    PLIOFreq=opts.OptPLIOFreq;
    PortBurst=opts.OptPortBurst;
    GMIOBW=opts.OptGMIOBW;
  }
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";
  
    if (!DMAToIO(mod,topFuncName))
      signalPassFailure();
  }

private:
  // Eliminate the dma ops that haven't been called by func.call
  // or haven't use the return value of func.call
  bool elimDMA(DmaOp op, Value val){
    // Check if it is the result of CallOp
    auto defineOp = val.getDefiningOp();
    if(defineOp && dyn_cast<CallOp>(defineOp))
      return false;
    // Check if it is used by CallOp
    for (auto user : val.getUsers())
      if(dyn_cast<CallOp>(user))
        return false;
    return true;
  }
  // Here need to deal with the data packing, and we assume the AIE local
  // mem is always row major
  bool dataPacking(OpBuilder builder, FuncOp topFunc, MemRefType type, 
                   int64_t portWidth, Value val, Value& finalVal, 
                   SmallVector<Value>& sizes, SmallVector<Value>& offsets, 
                   SmallVector<Type,8>& inTypes){
    auto loc = builder.getUnknownLoc();
    auto &entryBlock = topFunc.getBody().front();
    auto indexType = builder.getIndexType();
    auto shape = type.getShape();
    auto rank = type.getRank();
    auto typeWidth = type.getElementTypeBitWidth();
    auto packNum = (int)(portWidth / typeWidth);
    auto newTypeWidth = typeWidth * packNum;
    auto newType = builder.getIntegerType(newTypeWidth);
    finalVal = val;
    if (packNum!=1){
      builder.setInsertionPointToStart(&entryBlock);
      // Change the size of the DMA src/dst
      auto sizeNum = sizes.size();
      auto size = sizes[sizeNum-1];
      auto sizeDefineOp = size.getDefiningOp();
      auto constOp = dyn_cast<arith::ConstantOp>(sizeDefineOp);
      if(!constOp)
        return false;
      auto attr = constOp.getValue();
      int64_t lastSize;
      if (auto intAttr = dyn_cast<IntegerAttr>(attr)){
        auto intSize = intAttr.getInt();
        lastSize = intSize/packNum;
      }else{
        return false;
      }
      auto sizeAttr = builder.getIntegerAttr(indexType, lastSize);
      auto sizeValue 
           = builder.create<arith::ConstantOp>(loc, indexType, sizeAttr);
      sizes[sizeNum-1] = sizeValue;
      // Change the offset of the DMA src/dst
      auto offset = offsets[sizeNum-1];
      auto offsetDefineOp = offset.getDefiningOp();
      auto applyOp = dyn_cast<AffineApplyOp>(offsetDefineOp);
      if(!applyOp)
        return false;
      auto originalMap = applyOp.getAffineMap();
      SmallVector<AffineExpr, 4> modifiedExprs;
      for (auto expr : originalMap.getResults()) {
          auto dividedExpr = expr.floorDiv(2);
          modifiedExprs.push_back(dividedExpr);
      }
      auto newMap = AffineMap::get(originalMap.getNumDims(), 
                                   originalMap.getNumSymbols(), modifiedExprs, 
                                   applyOp.getContext());
      auto operands = applyOp.getOperands();
      builder.setInsertionPoint(applyOp);
      auto newApplyOp = builder.create<AffineApplyOp>(loc, newMap, operands);
      offsets[sizeNum-1] = newApplyOp.getResult();
      // Change the memref type of the val defined by AllocOp or in the Argus
      auto defineOp = val.getDefiningOp();
      if(defineOp){ // If the memref is defined by AllocOp
        if(auto allocOp = dyn_cast<AllocOp>(defineOp)){
          SmallVector<int64_t> sizesInt(shape.begin(),shape.end());
          sizesInt[rank-1] = sizesInt[rank-1] / packNum;
          builder.setInsertionPoint(allocOp);
          auto newMemRefType = MemRefType::get(sizesInt, newType);
          auto newAllocOp = builder.create<AllocOp>(loc, newMemRefType);
          finalVal = newAllocOp.getResult();
          val.replaceAllUsesWith(finalVal);
          allocOp.erase();
        }
        else{
          llvm::outs() << "Memref not defined by AllocOp\n";
          return false;
        }
      }else{ // Find the memref in the argument list
        unsigned idx = 0;
        for(auto arg : topFunc.getArguments()){
          if(arg == val){
            SmallVector<int64_t> sizesInt(shape.begin(),shape.end());
            sizesInt[rank-1] = sizesInt[rank-1] / packNum;
            auto newMemRefType = MemRefType::get(sizesInt, newType);
            inTypes[idx] = newMemRefType;
            arg.setType(newMemRefType);
            break;
          }
          idx++;
        }
      }
    }
    return true;
  }

  LogicalResult LowerDMAToIO(OpBuilder builder, FuncOp topFunc, 
                             std::string portType, 
                             int64_t portWidth, int64_t pliofreq, 
                             int64_t portBurst, int64_t gmiobw){
    auto context = builder.getContext();
    auto loc = builder.getUnknownLoc();
    auto inTypes =SmallVector<Type,8>(topFunc.getArgumentTypes().begin(),
                                      topFunc.getArgumentTypes().end());
    auto outTypes = topFunc.getResultTypes();
    auto flag = topFunc.walk([&](DmaOp op){
      auto DmaSrc = op.getSrc();
      auto DmaDst = op.getDst();
      auto SrcSpace 
           = DmaSrc.getType().dyn_cast<MemRefType>().getMemorySpaceAsInt();
      auto DstSpace 
           = DmaDst.getType().dyn_cast<MemRefType>().getMemorySpaceAsInt();
      auto srcType = dyn_cast<MemRefType>(DmaSrc.getType());
      auto dstType = dyn_cast<MemRefType>(DmaDst.getType());
      auto writeAttr = op->getAttr("write");
      auto readAttr = op->getAttr("read");

      GraphIOName portName;
      Type portIn, portOut;
      enum PortBurst portburst;
      unsigned flag_config=0;
      if (portType=="PLIO" || portType=="plio"){
        portName = GraphIOName::PLIO;
        portIn = PLIOType::get(context, PortDir::In, portWidth);
        portOut = PLIOType::get(context, PortDir::Out, portWidth);
        flag_config=1;
      }else if(portType=="GMIO" || portType=="gmio"){
        portName = GraphIOName::GMIO;
        portIn = GMIOType::get(context, PortDir::In);
        portOut = GMIOType::get(context, PortDir::Out);
        flag_config=2;
      }else if(portType=="PORT" || portType=="port"){
        portName = GraphIOName::PORT;
        portIn = PortType::get(context, PortDir::In);
        portOut = PortType::get(context, PortDir::Out);
      }else{
        return WalkResult::interrupt();
      }

      switch(portBurst) {
        case 0:
          portburst = PortBurst::BurstNULL;
          break;
        case 64:
          portburst = PortBurst::Burst64;
          break;
        case 128:
          portburst = PortBurst::Burst128;
          break;
        case 256:
          portburst = PortBurst::Burst256;
          break;
        default:
          portburst = PortBurst::Burst64;
      }

      //if the DmaOp is copied to L1 mem
      if(SrcSpace !=(int)MemorySpace::L1 && DstSpace == (int)MemorySpace::L1){
        // Elim dead DMAs
        if(elimDMA(op, DmaDst)){
          op.erase();
          return WalkResult::advance();
        }
        builder.setInsertionPoint(op);
        auto port 
             = builder.create<CreateGraphIOOp>(loc, portIn, portName);
        builder.setInsertionPointAfter(port);
        if(flag_config==1)
          builder.create<ConfigPLIOOp>(loc, port, pliofreq);
        else if(flag_config==2)
          builder.create<ConfigGMIOOp>(loc, port, portburst, gmiobw);
        auto dst = port.getResult();
        SmallVector<Value> src_offsets=op.getSrcOffsets();
        SmallVector<Value> src_sizes=op.getSrcSizes();
        SmallVector<Value> src_strides=op.getSrcStrides();
        builder.setInsertionPoint(op);
        auto newOp = builder.create<ConnectOp>(loc, port, DmaDst);
        op.erase();
        if(readAttr){
          auto intRAttr = dyn_cast<IntegerAttr>(readAttr);
          newOp->setAttr("read", intRAttr);
        }
        Value src;
        if(!dataPacking(builder, topFunc, srcType, portWidth, 
                        DmaSrc, src, src_sizes, src_offsets, inTypes))
          return WalkResult::interrupt();
        builder.setInsertionPoint(newOp);
        auto iopushOp = builder.create<IOPushOp>(loc, src, src_offsets,
                                                 src_sizes, src_strides, dst);
        auto elementType = srcType.getElementType();
        auto elementTypeAttr = TypeAttr::get(elementType);
        iopushOp->setAttr("type", elementTypeAttr);
        return WalkResult::advance();
      }else if(SrcSpace == (int)MemorySpace::L1 && 
               DstSpace != (int)MemorySpace::L1){
        // Elim dead DMAs
        if(elimDMA(op, DmaSrc)){
          op.erase();
          return WalkResult::advance();
        }
        builder.setInsertionPoint(op);
        auto port 
             = builder.create<CreateGraphIOOp>(loc,portOut,portName);
        builder.setInsertionPointAfter(port);
        if(flag_config==1)
          builder.create<ConfigPLIOOp>(loc, port, pliofreq);
        else if(flag_config==2)
          builder.create<ConfigGMIOOp>(loc, port, portburst, gmiobw);
        SmallVector<Value> dst_offsets=op.getDstOffsets();
        SmallVector<Value> dst_sizes=op.getDstSizes();
        SmallVector<Value> dst_strides=op.getDstStrides();
        builder.setInsertionPoint(op);
        auto newOp = builder.create<ConnectOp>(loc, DmaSrc, port);
        op.erase();
        if(writeAttr){
          auto intWAttr = dyn_cast<IntegerAttr>(writeAttr);
          newOp->setAttr("write", intWAttr);
        }
        Value dst;
        if(!dataPacking(builder, topFunc, dstType, portWidth, 
                        DmaDst, dst, dst_sizes, dst_offsets, inTypes))
          return WalkResult::interrupt();
        builder.setInsertionPointAfter(newOp);
        auto iopopOp = builder.create<IOPopOp>(loc, port, DmaDst, dst_offsets, 
                                               dst_sizes, dst_strides);
        auto elementType = dstType.getElementType();
        auto elementTypeAttr = TypeAttr::get(elementType);
        iopopOp->setAttr("type", elementTypeAttr);
        return WalkResult::advance();
      }else if(SrcSpace == (int)MemorySpace::L1 && 
               DstSpace == (int)MemorySpace::L1){
        builder.setInsertionPoint(op);
        builder.create<ConnectOp>(loc, DmaSrc, DmaDst);
        op.erase();
        return WalkResult::advance();
      }
      return WalkResult::advance();
    });
    topFunc.setType(builder.getFunctionType(inTypes, outTypes));

    if (flag == WalkResult::interrupt())
      return failure();
    return success();
  }

  LogicalResult BroadcastDetect(FuncOp topFunc){
    SmallVector<IOPushOp, 6> iopushs;
    topFunc.walk([&](IOPushOp op){
      iopushs.push_back(op);
    });

    // Record the index of IOpush that has the same source
    // And replace the dst with the dst of the first IOPush
    SmallVector<unsigned, 6> indeices;
    for (auto iopush0 : iopushs) {
      unsigned index = 0;
      for (auto iopush1 : iopushs) {
        if(iopush0==iopush1){
          index++;
          continue;
        }
        if (iopush0.getSrc() == iopush1.getSrc() &&
            iopush0.getSrcOffsets() == iopush1.getSrcOffsets() &&
            iopush0.getSrcSizes() == iopush1.getSrcSizes() &&
            iopush0.getSrcStrides() == iopush1.getSrcStrides()){
          auto dst1 = iopush1.getDst();
          auto dst0 = iopush0.getDst();
          if(dst0!=dst1){
            dst1.replaceAllUsesWith(dst0);
            indeices.push_back(index);
          }
        }
        index++;
      }
    }

    for (auto index : indeices) {
      auto op = iopushs[index];
      op.erase();
    }
    return success();
  }

  bool DMAToIO (ModuleOp mod, StringRef topFuncName) {
    auto builder = OpBuilder(mod);
    FuncOp topFunc;
    if(!topFind(mod, topFunc, topFuncName)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }

    auto attrGMIO = builder.getBoolAttr(false);
    auto attrPLIO = builder.getBoolAttr(false);
    if(PortType=="GMIO" || PortType=="gmio"){
      attrGMIO = builder.getBoolAttr(true);
    }else if(PortType=="PLIO" || PortType=="plio"){
      attrPLIO = builder.getBoolAttr(true);
    }
    topFunc->setAttr("gmio",attrGMIO);
    topFunc->setAttr("plio",attrPLIO);

    if (failed(LowerDMAToIO(builder, topFunc, PortType, PortWidth, PLIOFreq, 
                            PortBurst, GMIOBW)))
      return false;

    if (failed(BroadcastDetect(topFunc)))
      return false;

    return true;
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesDMAToIOPass() {
  return std::make_unique<AriesDMAToIO>();
}

std::unique_ptr<Pass> createAriesDMAToIOPass(const AriesOptions &opts) {
  return std::make_unique<AriesDMAToIO>(opts);
}

} // namespace aries
} // namespace mlir