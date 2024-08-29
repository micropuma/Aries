#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Transforms/DialectConversion.h"
#include "llvm/Support/Debug.h"
#include "aries/Conversion/Passes.h"
#include "aries/Transform/Utils.h"
#include "aries/Dialect/ADF/ADFDialect.h"

using namespace mlir;
using namespace aries;
using namespace adf;
using namespace mlir::func;
using namespace mlir::affine;

bool elimDMA(DmaOp op, Value val){
  for (auto user : val.getUsers())
    if(dyn_cast<CallOp>(user))
      return true;
  return false;
}

struct DmaConvert : public OpConversionPattern<DmaOp> {
    std::string portType;
    int64_t portWidth;
    int64_t pliofreq;
    int64_t portBurst;
    int64_t gmiobw;
    DmaConvert(MLIRContext *context, std::string portType, 
               int64_t portWidth, int64_t pliofreq, 
               int64_t portBurst, int64_t gmiobw)
        : OpConversionPattern<DmaOp>(context), portType(portType),
                                     portWidth(portWidth), pliofreq(pliofreq),
                                     portBurst(portBurst), gmiobw(gmiobw){}
    LogicalResult matchAndRewrite(
        DmaOp op, OpAdaptor adaptor,
        ConversionPatternRewriter &rewriter) const override {

      auto DmaSrc = op.getSrc();
      auto DmaDst = op.getDst();
      auto SrcSpace 
           = DmaSrc.getType().dyn_cast<MemRefType>().getMemorySpaceAsInt();
      auto DstSpace 
           = DmaDst.getType().dyn_cast<MemRefType>().getMemorySpaceAsInt();
      auto writeAttr = op->getAttr("write");
      auto readAttr = op->getAttr("read");

      GraphIOName portName;
      Type portIn,portOut;
      PortBurst portburst;
      unsigned flag_config=0;
      if (portType=="PLIO" || portType=="plio"){
        portName = GraphIOName::PLIO;
        portIn = PLIOType::get(rewriter.getContext(), PortDir::In, portWidth);
        portOut = PLIOType::get(rewriter.getContext(), PortDir::Out, portWidth);
        flag_config=1;
      }else if(portType=="GMIO" || portType=="gmio"){
        portName = GraphIOName::GMIO;
        portIn = GMIOType::get(rewriter.getContext(), PortDir::In);
        portOut = GMIOType::get(rewriter.getContext(), PortDir::Out);
        flag_config=2;
      }else if(portType=="PORT" || portType=="port"){
        portName = GraphIOName::PORT;
        portIn = PortType::get(rewriter.getContext(), PortDir::In);
        portOut = PortType::get(rewriter.getContext(), PortDir::Out);
      }else{
        return failure();
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
          rewriter.eraseOp(op);
          return success();
        }
        rewriter.setInsertionPoint(op);
        auto port 
             = rewriter.create<CreateGraphIOOp>(op->getLoc(), portIn, portName);
        rewriter.setInsertionPointAfter(port);
        if(flag_config==1)
          rewriter.create<ConfigPLIOOp>(port->getLoc(), port, pliofreq);
        else if(flag_config==2)
          rewriter.create<ConfigGMIOOp>(port->getLoc(), port, portburst,gmiobw);
        auto dst = port.getResult();
        SmallVector<Value> src_offsets=op.getSrcOffsets();
        SmallVector<Value> src_sizes=op.getSrcSizes();
        SmallVector<Value> src_strides=op.getSrcStrides();
        auto newOp = rewriter.replaceOpWithNewOp<ConnectOp>(op, port, DmaDst);
        if(readAttr){
          auto intRAttr = dyn_cast<IntegerAttr>(readAttr);
          newOp->setAttr("read", intRAttr);
        }
        rewriter.setInsertionPoint(newOp);
        rewriter.create<IOPushOp>(newOp->getLoc(), DmaSrc, src_offsets,
                                  src_sizes, src_strides, dst);
        return success();
      }else if(SrcSpace == (int)MemorySpace::L1 && 
               DstSpace != (int)MemorySpace::L1){
        // Elim dead DMAs
        if(elimDMA(op, DmaSrc)){
          rewriter.eraseOp(op);
          return success();
        }
        rewriter.setInsertionPoint(op);
        auto port 
             = rewriter.create<CreateGraphIOOp>(op->getLoc(),portOut,portName);
        rewriter.setInsertionPointAfter(port);
        if(flag_config==1)
          rewriter.create<ConfigPLIOOp>(port->getLoc(), port, pliofreq);
        else if(flag_config==2)
          rewriter.create<ConfigGMIOOp>(port->getLoc(), port, portburst,gmiobw);
        SmallVector<Value> dst_offsets=op.getDstOffsets();
        SmallVector<Value> dst_sizes=op.getDstSizes();
        SmallVector<Value> dst_strides=op.getDstStrides();
        auto newOp = rewriter.replaceOpWithNewOp<ConnectOp>(op, DmaSrc, port);
        if(writeAttr){
          auto intWAttr = dyn_cast<IntegerAttr>(writeAttr);
          newOp->setAttr("write", intWAttr);
        }
        rewriter.setInsertionPointAfter(newOp);
        rewriter.create<IOPopOp>(newOp->getLoc(), port, DmaDst, 
                                 dst_offsets, dst_sizes, dst_strides);
        return success();
      }else if(SrcSpace == (int)MemorySpace::L1 && 
               DstSpace == (int)MemorySpace::L1){
        rewriter.replaceOpWithNewOp<ConnectOp>(op, DmaSrc, DmaDst);
        return success();
      }
      return failure();
    }
  };

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
    MLIRContext &context = getContext();
    RewritePatternSet patterns(&context);

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

    ConversionTarget target(context);
    target.addIllegalOp<DmaOp>();
    patterns.add<DmaConvert>(patterns.getContext(), PortType,
                             PortWidth, PLIOFreq, PortBurst, GMIOBW);
    target.addLegalOp<CreateGraphIOOp>();
    target.addLegalOp<IOPushOp>();
    target.addLegalOp<IOPopOp>();
    target.addLegalOp<ConnectOp>();
    target.addLegalOp<AffineApplyOp>();
    target.addLegalDialect<ADFDialect>();

    if (failed(applyPartialConversion(mod, target, std::move(patterns))))
      return false;

    if (failed(BroadcastDetect(topFunc)))
      return false;

    return true;
  }

};
} // namespace



namespace mlir {
namespace aries {
namespace adf {

std::unique_ptr<Pass> createAriesDMAToIOPass() {
  return std::make_unique<AriesDMAToIO>();
}

std::unique_ptr<Pass> createAriesDMAToIOPass(const AriesOptions &opts) {
  return std::make_unique<AriesDMAToIO>(opts);
}

} // namespace adf
} // namespace aries
} // namespace mlir