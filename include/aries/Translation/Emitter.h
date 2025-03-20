#ifndef ARIES_TRANSLATION_EMITTER_H
#define ARIES_TRANSLATION_EMITTER_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/InitAllDialects.h"
#include "aries/Dialect/ADF/ADFDialect.h"


namespace mlir {
namespace aries {

// 主体代码生成code
LogicalResult emitAriesCpp(ModuleOp module, llvm::raw_ostream &os);
void registerEmitAriesCppTranslation();

/// Translates the AIE vector dialect MLIR to C++ code for Vitis Flow.
/// 将AIE的向量方言mlir ir翻译成vitis的flow code
LogicalResult emitAIEVecToCpp(mlir::Operation *op, bool aieml,
                              bool vitis, bool enres, mlir::raw_ostream &os);
// 生成c++ kernel端代码
LogicalResult emitKernelFunc(ModuleOp module, llvm::raw_ostream &os);
void registeremitKernelFuncTranslation();

/// Register all exporters.
void 代码();

//===----------------------------------------------------------------------===//
// Base Classes of emitters
//===----------------------------------------------------------------------===//

/// This class maintains the mutable state that cross-cuts and is shared by the
/// various emitters.
/// 代码生成器的状态结构体
class ADFEmitterState {
public:
  explicit ADFEmitterState(raw_ostream &os) : os(os) {}

  // The stream to emit to.
  raw_ostream &os;

  bool encounteredError = false;
  unsigned currentIndent = 0;

  // This table contains all declared values.
  DenseMap<Value, SmallString<8>> nameTable;
  DenseMap<func::CallOp, SmallString<8>> callTable;
  std::map<std::string, int> nameConflictCnt;
  SmallVector<unsigned, 3> idxCnts={0,0,0};

private:
  ADFEmitterState(const ADFEmitterState &) = delete;
  void operator=(const ADFEmitterState &) = delete;
};

/// This is the base class for all of the ADFCpp emitters.
class ADFEmitterBase {
public:
  explicit ADFEmitterBase(ADFEmitterState &state): state(state), os(state.os) {}

  // 显示控制缩进
  raw_ostream &indent() { return os.indent(state.currentIndent); }

  void addIndent() { state.currentIndent += 2; }
  void reduceIndent() { state.currentIndent -= 2; }

  // All of the mutable state we are maintaining.
  ADFEmitterState &state;

  // The stream to emit to.
  raw_ostream &os;

  /// Value name management methods.
  SmallString<8> addName(Value val, bool isPtr = false, std::string name = "");

  SmallString<8> addCall(func::CallOp call, std::string name = "");

  SmallString<8> getName(Value val);

  SmallString<8> getCall(func::CallOp call);

  // 针对DMA的操作，生成对应的字符串
  SmallString<16> getDMAAccess(adf::DmaOp op, unsigned rank, 
                               bool isSrc, bool dir);
  
  SmallString<16> getTypeName(Type valType, bool isPtr = false,
                              bool BIT_FLAG = false);
  
  SmallString<16> getTypeName(func::CallOp call);

  SmallString<16> getTypeName(Value val, bool isPtr = false, 
                              bool BIT_FLAG = false);



  // 查询state中的name table，查看是否已经声明过
  bool isDeclared(Value val) {
    if (getName(val).empty()) {
      return false;
    } else
      return true;
  }

  // 查询state中的call table，查看是否已经声明过
  bool isDeclared(func::CallOp call) {
    if (getCall(call).empty()) {
      return false;
    } else
      return true;
  }

private:
  ADFEmitterBase(const ADFEmitterBase &) = delete;
  void operator=(const ADFEmitterBase &) = delete;
};



} // namespace aries
} // namespace mlir

#endif // ARIES_TRANSLATION_EMITTER_H