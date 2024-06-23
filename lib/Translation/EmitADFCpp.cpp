#include "mlir/InitAllDialects.h"
#include "mlir/Tools/mlir-translate/Translation.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "aries/Translation/EmitADFCpp.h"
#include "aries/Transform/Utils.h"
#include "aries/Dialect/ADF/ADFDialect.h"

using namespace mlir;
using namespace aries;
using namespace adf;
using namespace mlir::func;


// TODO: update naming rule.
SmallString<8> ADFEmitterBase::addName(Value val, bool isPtr,
                                       std::string name) {
  assert(!isDeclared(val) && "has been declared before.");

  SmallString<8> valName;
  if (isPtr)
    valName += "*";

  if (name != "") {
    if (state.nameConflictCnt.count(name) > 0) {
      state.nameConflictCnt[name]++;
      valName += StringRef(name + std::to_string(state.nameConflictCnt[name]));
    } else { // first time
      state.nameConflictCnt[name] = 0;
      valName += name;
    }
  } else {
    valName += StringRef("v" + std::to_string(state.nameTable.size()));
  }
  state.nameTable[val] = valName;

  return valName;
};

SmallString<8> ADFEmitterBase::getName(Value val) {
  // For constant scalar operations, the constant number will be returned
  // rather than the value name.
  if (auto defOp = val.getDefiningOp()) {
    if (auto constOp = dyn_cast<arith::ConstantOp>(defOp)) {
      auto constAttr = constOp.getValue();

      if (auto boolAttr = constAttr.dyn_cast<BoolAttr>()) {
        return SmallString<8>(std::to_string(boolAttr.getValue()));

      } else if (auto floatAttr = constAttr.dyn_cast<FloatAttr>()) {
        auto value = floatAttr.getValueAsDouble();
        if (std::isfinite(value))
          return SmallString<8>(std::to_string(value));
        else if (value > 0)
          return SmallString<8>("INFINITY");
        else
          return SmallString<8>("-INFINITY");

      } else if (auto intAttr = constAttr.dyn_cast<IntegerAttr>()) {
        auto value = intAttr.getInt();
        return SmallString<8>(std::to_string(value));
      }
    }
  }
  return state.nameTable.lookup(val);
};

static SmallString<16> getType(Type type) {
  if (auto plioType = dyn_cast<PLIOType>(type)){
    auto Dir = plioType.getDir();
    if(Dir==PortDir::In)
      return SmallString<16>("input_plio");
    else if(Dir==PortDir::Out)
      return SmallString<16>("output_plio");
    else
      assert("PLIO can't be an inout port.");
  }else if (auto gmioType = dyn_cast<GMIOType>(type)){
    auto Dir = gmioType.getDir();
    if(Dir==PortDir::In)
      return SmallString<16>("input_gmio");
    else if(Dir==PortDir::Out)
      return SmallString<16>("output_gmio");
    else
      assert("GMIO can't be an inout port.");
  }else if (auto portType = dyn_cast<PortType>(type)){
    auto Dir = portType.getDir();
    if(Dir==PortDir::In)
      return SmallString<16>("input_port");
    else if(Dir==PortDir::Out)
      return SmallString<16>("output_port");
    else
      return SmallString<16>("inout_port");
  }else{
    assert("Dectect data type not supported.");
  }
}

namespace {
class ADFEmitter : public ADFEmitterBase {
public:
  explicit ADFEmitter(ADFEmitterState &state) : ADFEmitterBase(state) {}
  /// Top-level MLIR module emitter.
  void emitModule(ModuleOp module);
private:
  void emitADFGraphHeader(FuncOp func);
};
}

void ADFEmitter::emitADFGraphHeader(FuncOp func) {
  auto GraphName = func.getName();
  os << "class " << func.getName() << ": public adf::graph{\n";
  os << "private:\n";

  llvm::SmallVector<std::pair<StringRef, unsigned>, 4> calleeCounts;
  func.walk([&](CallOp call){
    auto calleeName = call.getCallee();
    bool found = false;
    for (auto &entry : calleeCounts) {
      if (entry.first == calleeName) {
        entry.second++;
        std::string KName = entry.first.str() + std::to_string(entry.second);
        os << "  adf::kernel " << KName << ";\n";
        found = true;
        break;
      }
    }
    if (!found) {
      calleeCounts.push_back(std::make_pair(calleeName, 0));
      std::string KName = calleeName.str() + std::to_string(0);
      os << "  adf::kernel " << KName << ";\n";
    }
  });

  
  os << "public:\n";

  llvm::SmallVector<std::pair<StringRef, unsigned>, 4> ioCounts;

}

void ADFEmitter::emitModule(ModuleOp module) {
  std::string adf_header = R"XXX(
//===----------------------------------------------------------------------===//
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


)XXX";

  for (auto op : module.getOps<FuncOp>()) {
      if (op->getAttr("adf.graph")){
        os << adf_header;
        emitADFGraphHeader(op);
      } 
  }

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