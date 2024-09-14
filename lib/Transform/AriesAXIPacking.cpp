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

struct AriesAXIPacking 
       : public AriesAXIPackingBase<AriesAXIPacking> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";

    if (!AXIPacking(mod,topFuncName))
      signalPassFailure();
  }

private:
  // This function checks if innermost loop is used by the last dim of affineMap
  // of an AffineLoadOp/AffineSoreOp, and the coeff of the innermost loop = 1
  // Then update the last dim of affineMap and the upperbound of innermost
  bool getPackList(Value arg, unsigned packNum, size_t& numUser,
                   SmallVector<std::pair<AffineForOp, int64_t>, 4>& loopList,
                   SmallVector<std::pair<Operation*, AffineMap>, 4>& packList){
    auto &context = getContext();
    for(auto user : arg.getUsers()){
      numUser++;
      AffineMap map;
      SmallVector<Value> operands;
      Operation* affineOp;
      SmallVector<AffineForOp, 6> band;
      if (auto read = dyn_cast<AffineLoadOp>(user)) {
        map = read.getAffineMap();
        operands = read.getMapOperands();
        affineOp = read;
      }else if(auto write = dyn_cast<AffineStoreOp>(user)){
        map = write.getAffineMap();
        operands = write.getMapOperands();
        affineOp = write;
      }else{
        continue;
      }
      getSurroundingLoops(*affineOp, band);
      auto mapSize = map.getNumResults();
      auto dimSize = map.getNumDims();
      auto symSize = map.getNumSymbols();
      auto lastDimExpr = map.getResult(mapSize-1);
      // flattened form [dims, symbols, locals, constant]
      llvm::SmallVector<int64_t> flattenedExpr;
      llvm::SmallVector<int64_t> newFlattenedExpr;
      if (failed(getFlattenedAffineExpr(lastDimExpr, dimSize, symSize,
                                        &flattenedExpr)))
        return false;
      // Check if innerLoop has coeff == 1 && Update AffineMap
      for (unsigned i = 0; i < dimSize; ++i) {
        auto loop = getForInductionVarOwner(operands[i]);
        auto it = llvm::find(band, loop);
        if(it==band.end())
          return false;
        auto coeff = flattenedExpr[i];
        unsigned depth = it - band.begin();
        if(depth != (unsigned)(band.size()-1)){
          if(coeff%packNum!=0)
            return false;
          auto newCoeff = (int64_t)(coeff / packNum);
          newFlattenedExpr.push_back(newCoeff);
        }else{
          auto upbound = loop.getConstantUpperBound();
          auto newBound = (int64_t)(upbound/packNum);
          loopList.push_back(std::pair(loop, newBound));
          if(coeff!=1)
            return false;
          newFlattenedExpr.push_back(coeff);
        }
      }
      auto exprSize = flattenedExpr.size();
      for(unsigned i = dimSize; i < exprSize; ++i){
        auto coeff = flattenedExpr[i];
        if(coeff%packNum!=0)
          return false;
        auto newCoeff = (int64_t)(coeff / packNum);
        newFlattenedExpr.push_back(newCoeff);
      }
      // Get new experssion for the last dim
      auto newLastExpr = getAffineExprFromFlatForm(newFlattenedExpr, dimSize,
                                                   symSize, {}, &context);
      // Update affineMap
      SmallVector<AffineExpr, 4> newExprs(map.getResults().begin(), 
                                          map.getResults().end());
      newExprs.back() = newLastExpr;
      auto newMap = AffineMap::get(dimSize, symSize, newExprs, &context);
      packList.push_back(std::pair(affineOp, newMap));
    }
    return true;
  }

  void applyPacking(OpBuilder builder, BlockArgument arg, unsigned index,
                    MemRefType type, unsigned packNum,
                    SmallVector<Type,8>& inTypes,
                    SmallVector<std::pair<AffineForOp, int64_t>, 4> loopList,
                    SmallVector<std::pair<Operation*, AffineMap>, 4> packlist,
                    SmallVector<AffineLoadOp, 4>& eliminateOps){
    auto loc = builder.getUnknownLoc();
    auto shape = type.getShape();
    auto rank = type.getRank();
    auto elemType = type.getElementType();
    auto typeWidth = type.getElementTypeBitWidth();
    auto newTypeWidth = typeWidth * packNum;
    auto newType = builder.getIntegerType(newTypeWidth);
    // Update innerLoop Upperbound
    for(auto loopPair : loopList){
      auto loop = loopPair.first;
      auto ub = loopPair.second;
      loop.setConstantUpperBound(ub);
    }

    // Update arguments in func
    SmallVector<int64_t> sizesInt(shape.begin(),shape.end());
    sizesInt[rank-1] = sizesInt[rank-1] / packNum;
    auto newMemRefType = MemRefType::get(sizesInt, newType);
    inTypes[index] = newMemRefType;
    arg.setType(newMemRefType);

    // Update access map && add logic to send new data to stream
    for(auto packPair : packlist){
      auto op = packPair.first;
      auto map = packPair.second;
      if(auto read = dyn_cast<AffineLoadOp>(op)){
        // Update the access map
        read.setMap(map);
        auto result = read.getResult();
        auto operands = read.getOperands();
        SmallVector<Value, 4> indices(operands.begin() + 1, operands.end()); 
        builder.setInsertionPoint(read);
        auto newRead = builder.create<AffineLoadOp>(loc, newType, arg, indices, map);
        eliminateOps.push_back(read);
        auto newResult = newRead.getResult(); 
        // Insert GetIntSliceOp to transfer data from newType to oldType 
        builder.setInsertionPointAfter(newRead);
        auto forOp = builder.create<AffineForOp>(loc, 0, packNum);
        auto entryBlock = forOp.getBody();
        auto forYiledOp = dyn_cast<AffineYieldOp>(entryBlock->getTerminator());
        builder.setInsertionPoint(forYiledOp);
        // Create the ub for slicing
        auto hiExpr = builder.getAffineDimExpr(0) * typeWidth + typeWidth-1;
        auto hiMap = AffineMap::get(1, 0, hiExpr);
        SmallVector<Value, 1> hiOperands = {forOp.getInductionVar()};
        auto hiVal = builder.create<AffineApplyOp>(loc, hiMap, hiOperands);
        // Create the lb for slicing
        auto loExpr = builder.getAffineDimExpr(0) * typeWidth;
        auto loMap = AffineMap::get(1, 0, loExpr);
        SmallVector<Value, 1> loOperands = {forOp.getInductionVar()};
        auto loVal = builder.create<AffineApplyOp>(loc, loMap, loOperands);
        auto slice = builder.create<GetIntSliceOp>(loc, elemType, newResult,
                                                   hiVal, loVal);
        // Move the use into the AffineForOp
        SmallVector<Operation*, 4> users;
        for(auto use: result.getUsers()){
          use->moveBefore(forYiledOp);
        }
        result.replaceAllUsesWith(slice);
      }else if(auto write = dyn_cast<AffineStoreOp>(op)){
        // Update the access map
        write.setMap(map);
        auto val = write.getValueToStore();
        // Insert SetIntSliceOp to transfer data from oldType to newType 
        builder.setInsertionPoint(write);
        auto forOp = builder.create<AffineForOp>(loc, 0, packNum);
        auto entryBlock = forOp.getBody();
        auto forYiledOp = dyn_cast<AffineYieldOp>(entryBlock->getTerminator());
        // Create temp reg to store newType
        builder.setInsertionPoint(forOp);
        auto attr = builder.getIntegerAttr(newType, 0);
        auto zeroVal = builder.create<arith::ConstantOp>(loc, newType, attr);
        auto temp = builder.create<IntToAPInt>(loc, newType, zeroVal);
        builder.setInsertionPoint(forYiledOp);
        // Create the ub for slicing
        auto hiExpr = builder.getAffineDimExpr(0) * typeWidth + typeWidth-1;
        auto hiMap = AffineMap::get(1, 0, hiExpr);
        SmallVector<Value, 1> hiOperands = {forOp.getInductionVar()};
        auto hiVal = builder.create<AffineApplyOp>(loc, hiMap, hiOperands);
        // Create the lb for slicing
        auto loExpr = builder.getAffineDimExpr(0) * typeWidth;
        auto loMap = AffineMap::get(1, 0, loExpr);
        SmallVector<Value, 1> loOperands = {forOp.getInductionVar()};
        auto loVal = builder.create<AffineApplyOp>(loc, loMap, loOperands);
        builder.create<SetIntSliceOp>(loc, temp, hiVal, loVal, val);
        write.setOperand(0, temp);
      }
    }             
  }
  
  // This function pack the L3 memory to specified port width in order to
  // increase the L3 access bandwidth
  void mergeAXIPort (OpBuilder builder,FuncOp& plFunc, unsigned axiWidth,
                     SmallVector<unsigned, 4>& argList){
    auto inTypes = SmallVector<Type,8>(plFunc.getArgumentTypes().begin(),
                                       plFunc.getArgumentTypes().end());
    auto outTypes = plFunc.getResultTypes();
    SmallVector<AffineLoadOp, 4> eliminateOps;
    auto argNum= plFunc.getNumArguments();
    for(unsigned i = 0; i < argNum; i++){
      auto arg = plFunc.getArgument(i);
      if(!dyn_cast<MemRefType>(arg.getType()))
        continue;
      auto type = dyn_cast<MemRefType>(arg.getType());
      if(auto memSpace = type.getMemorySpace()){
        auto intSpace = dyn_cast<IntegerAttr>(memSpace);
        if(!intSpace || intSpace.getInt()!=(int)MemorySpace::L3)
          continue;
      }
      auto typeWidth = type.getElementTypeBitWidth();
      auto packNum = (unsigned)(axiWidth / typeWidth); 
      if(packNum<=1)
        continue;
      SmallVector<std::pair<AffineForOp, int64_t>, 4> loopList;
      SmallVector<std::pair<Operation*, AffineMap>, 4> packlist;
      size_t numUser = 0;
      if(!getPackList(arg, packNum, numUser, loopList, packlist))
        continue;
      
      if(loopList.size()!=packlist.size() || loopList.size()!= numUser)
        continue;
      
      applyPacking(builder, arg, i, type, packNum, inTypes, loopList, packlist, eliminateOps);
      argList.push_back(i);
    }
    for(auto read: llvm::make_early_inc_range(eliminateOps))
      read.erase();
    plFunc.setType(builder.getFunctionType(inTypes, outTypes));
  }

  // Verify if the number of the use of the argument by callOps
  // equals the time need to be changed.
  // This is to gurantee all the callOp change the argument type
  bool topFuncUpdate(OpBuilder builder, FuncOp& topFunc, unsigned axiWidth,
                     std::vector<unsigned> cnt){
    auto inTypes = SmallVector<Type,8>(topFunc.getArgumentTypes().begin(),
                                       topFunc.getArgumentTypes().end());
    auto outTypes = topFunc.getResultTypes();
    auto argNum = cnt.size();
    std::vector<unsigned> callCnt(argNum, 0);
    for(unsigned index=0; index < argNum; index++){
      auto arg = topFunc.getArgument(index);
      for(auto use: arg.getUsers()){
        if(auto call = dyn_cast<CallOp>(use)){
          callCnt[index]++;
        }
      }
    }
    for(unsigned index=0; index < argNum; index++){
      if(callCnt[index] != cnt[index])
        return false;
      if(callCnt[index]==0)
        continue;
      auto arg = topFunc.getArgument(index);
      if(!dyn_cast<MemRefType>(arg.getType()))
        continue;
      auto type = dyn_cast<MemRefType>(arg.getType());
      auto shape = type.getShape();
      auto rank = type.getRank();
      if(auto memSpace = type.getMemorySpace()){
        auto intSpace = dyn_cast<IntegerAttr>(memSpace);
        if(!intSpace || intSpace.getInt()!=(int)MemorySpace::L3)
          continue;
      }
      auto typeWidth = type.getElementTypeBitWidth();
      auto packNum = (unsigned)(axiWidth / typeWidth); 
      auto newType = builder.getIntegerType(axiWidth);
      if(packNum<=1)
        continue;
      // Update arguments in topFunc
      SmallVector<int64_t> sizesInt(shape.begin(),shape.end());
      sizesInt[rank-1] = sizesInt[rank-1] / packNum;
      auto newMemRefType = MemRefType::get(sizesInt, newType);
      inTypes[index] = newMemRefType;
      arg.setType(newMemRefType);
    }
    topFunc.setType(builder.getFunctionType(inTypes, outTypes));
    return true;
  }

  bool AXIPacking (ModuleOp mod, StringRef topFuncName) {
    auto builder = OpBuilder(mod);
    FuncOp topFunc;
    if(!topFind(mod, topFunc, topFuncName)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }
    unsigned argNum = topFunc.getNumArguments();
    unsigned axiWidth = 512;
    std::vector<unsigned> cnt(argNum, 0);
    mod.walk([&](FuncOp func){
      if(!func->hasAttr("adf.pl"))
        return WalkResult::advance();
      SmallVector<unsigned, 4> argList;
      mergeAXIPort(builder, func, axiWidth, argList);
      auto calleeName = func.getName();
      topFunc.walk([&](CallOp call){
        auto symbolName = call.getCallee();
        if(calleeName!=symbolName)
          return WalkResult::advance();
        for(auto idx : argList){
          auto operand = call.getOperand(idx);
          for(unsigned index=0; index < argNum; index++){
            auto arg = topFunc.getArgument(index);
            if(operand != arg)
              continue;
            cnt[index]++;
            break;
          }
        }
        return WalkResult::advance();
      });
      return WalkResult::advance();
    });
    if(!topFuncUpdate(builder, topFunc, axiWidth, cnt))
      return false;
    return true;
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesAXIPackingPass() {
  return std::make_unique<AriesAXIPacking>();
}

} // namespace aries
} // namespace mlir