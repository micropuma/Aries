#include "aries/Translation/Emitter.h"
#include "mlir/Tools/mlir-translate/Translation.h"
#include "aries/Dialect/ADF/ADFDialect.h"

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

void aries::registerAriesEmitters() {
  registerEmitADFCppTranslation();
  registeremitKernelHeaderTranslation();
}