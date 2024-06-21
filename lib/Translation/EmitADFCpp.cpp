#include "mlir/InitAllDialects.h"
#include "mlir/Tools/mlir-translate/Translation.h"
#include "llvm/Support/raw_ostream.h"
#include "aries/Translation/EmitADFCpp.h"
#include "aries/Transform/Utils.h"
#include "aries/Dialect/ADF/ADFDialect.h"

using namespace mlir;
using namespace aries;
using namespace adf;

namespace {
class ADFEmitter : public ADFEmitterBase {
public:
  explicit ADFEmitter(ADFEmitterState &state) : ADFEmitterBase(state) {}
  /// Top-level MLIR module emitter.
  void emitModule(ModuleOp module);
};
}

void ADFEmitter::emitModule(ModuleOp module) {
std::string adf_header = R"XXX(
//===------------------------------------------------------------*- C++ -*-===//
//
// Automatically generated file for adf graph
//
//===----------------------------------------------------------------------===//
#ifndef __GRAPH_L0_H__
#define __GRAPH_L0_H__

#include <adf.h>
#include "para_L0.h"
#include <stdio.h>
using namespace adf;

#endif /**********__GRAPH_H__**********/
)XXX";

os << adf_header;

}

LogicalResult aries::emitADFCpp(ModuleOp module, llvm::raw_ostream &os) {
  ADFEmitterState state(os);
  ADFEmitter(state).emitModule(module);
  return failure(state.encounteredError);
}

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