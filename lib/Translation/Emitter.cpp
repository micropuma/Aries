#include "aries/Translation/Emitter.h"
#include "mlir/Tools/mlir-translate/Translation.h"
#include "aries/Dialect/ADF/ADFDialect.h"
#include "aie/Dialect/AIE/IR/AIEDialect.h"
#include "aie/Dialect/AIEVec/IR/AIEVecOps.h"

using namespace mlir;
using namespace aries;


void aries::registerEmitADFCppTranslation() {
  static TranslateFromMLIRRegistration registration(
      "emit-aie-adf", "Emit AIE ADF Graph", emitADFCpp,
      [&](DialectRegistry &registry) {
        registry.insert<
          mlir::aries::adf::ADFDialect,
          mlir::func::FuncDialect,
          mlir::affine::AffineDialect,
          mlir::memref::MemRefDialect
        >();
      });
}

void aries::registeremitKernelHeaderTranslation() {
  static TranslateFromMLIRRegistration registration(
      "emit-kenrel-header", "Emit ADF Kernel Header", emitKernelHeader,
      [&](DialectRegistry &registry) {
        registry.insert<
          mlir::aries::adf::ADFDialect,
          mlir::func::FuncDialect,
          mlir::affine::AffineDialect,
          mlir::memref::MemRefDialect
        >();
      });
}

void aries::registeremitKernelFuncTranslation() {
  static TranslateFromMLIRRegistration registration(
      "emit-kenrel-func", "Emit ADF Kernel Func", emitKernelFunc,
      [&](DialectRegistry &registry) {
        registry.insert<
          mlir::func::FuncDialect,
          mlir::affine::AffineDialect,
          mlir::memref::MemRefDialect,
          mlir::scf::SCFDialect,
          mlir::vector::VectorDialect,
          xilinx::AIE::AIEDialect,
          xilinx::aievec::AIEVecDialect
        >();
      });
}

void aries::registerAriesEmitters() {
  registerEmitADFCppTranslation();
  registeremitKernelHeaderTranslation();
  registeremitKernelFuncTranslation();
}