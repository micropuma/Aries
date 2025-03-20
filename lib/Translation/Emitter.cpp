#include "mlir/Tools/mlir-translate/Translation.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/DLTI/DLTI.h"
#include "mlir/Dialect/EmitC/IR/EmitC.h"
#include "mlir/Dialect/Index/IR/IndexDialect.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/Math/IR/Math.h"
#include "mlir/Dialect/Vector/IR/VectorOps.h"
#include "aries/Translation/Emitter.h"
#include "aries/Dialect/ADF/ADFDialect.h"
#include "aie/Dialect/AIEVec/AIE1/IR/AIEVecAIE1Ops.h"
#include "aie/Dialect/AIEVec/IR/AIEVecDialect.h"

using namespace mlir;
using namespace aries;

// 生成AIE的单个核心的kernel函数，aie的adf graph链接以及vitis hls代码。
void aries::registerEmitAriesCppTranslation() {
  // mlir的plugin机制
  static TranslateFromMLIRRegistration registration(
  "emit-aries-kernels", "Emit AIE Single Kernel + AIE ADF Graph + Vitis HLS", 
  emitAriesCpp,
  [&](DialectRegistry &registry) {
    registry.insert<
      mlir::aries::adf::ADFDialect,
      mlir::func::FuncDialect,
      mlir::affine::AffineDialect,
      mlir::memref::MemRefDialect,
      mlir::arith::ArithDialect,
      mlir::memref::MemRefDialect,
      mlir::scf::SCFDialect,
      mlir::vector::VectorDialect,
      arith::ArithDialect,
      emitc::EmitCDialect,
      LLVM::LLVMDialect,
      math::MathDialect,
      cf::ControlFlowDialect,
      DLTIDialect,
      xilinx::aievec::aie1::AIEVecAIE1Dialect,
      xilinx::aievec::AIEVecDialect,
      index::IndexDialect
    >();
  });
}

// 生成ADF的核函数
void aries::registeremitKernelFuncTranslation() {
  static TranslateFromMLIRRegistration registration(
      "emit-kenrel-func", "Emit ADF Kernel Func", emitKernelFunc,
      [&](DialectRegistry &registry) {
        registry.insert<
          mlir::aries::adf::ADFDialect,
          mlir::func::FuncDialect,
          mlir::affine::AffineDialect,
          mlir::memref::MemRefDialect,
          mlir::scf::SCFDialect,
          mlir::vector::VectorDialect,
          arith::ArithDialect,
          emitc::EmitCDialect,
          LLVM::LLVMDialect,
          math::MathDialect,
          cf::ControlFlowDialect,
          DLTIDialect,
          xilinx::aievec::aie1::AIEVecAIE1Dialect,
          xilinx::aievec::AIEVecDialect,
          index::IndexDialect
        >();
      });
}


void aries::registerAriesEmitters() {
  registerEmitAriesCppTranslation();
  registeremitKernelFuncTranslation();
}