#include "mlir/InitAllDialects.h"
#include "mlir/Tools/mlir-translate/Translation.h"
#include "aries/Translation/Emitter.h"
#include "aries/Transform/Utils.h"
#include "aries/Dialect/ADF/ADFDialect.h"

static SmallString<16> getTypeName(Type valType) {
  if (auto arrayType = dyn_cast<ShapedType>(valType))
    valType = arrayType.getElementType();

  // Handle float types.
  if (valType.isa<Float32Type>())
    return SmallString<16>("float");
  else if (valType.isa<Float64Type>())
    return SmallString<16>("double");

  // Handle integer types.
  else if (valType.isa<IndexType>())
    return SmallString<16>("int");
  else if (auto intType = dyn_cast<IntegerType>(valType)) {
    if (intType.getWidth() == 1) {
      return SmallString<16>("bool");
    } else {
      std::string signedness = "";
      if (intType.getSignedness() == IntegerType::SignednessSemantics::Unsigned)
        signedness = "u";
      switch (intType.getWidth()) {
      case 8:
      case 16:
      case 32:
      case 64:
        return SmallString<16>(signedness + "int" +
                               std::to_string(intType.getWidth()) + "_t");
      default:
        return SmallString<16>(signedness + "int" +
                               std::to_string(intType.getWidth()) + "_t");
      }
    }
  }
  return SmallString<16>();
}

static SmallString<16> getTypeName(Value val) {
  auto valType = val.getType();
  return getTypeName(valType);
}

LogicalResult aries::emitKernelHeader(ModuleOp module, llvm::raw_ostream &os) {
  std::string kernel_header = R"XXX(
#ifndef __KERNEL_H__
#define __KERNEL_H__
using namespace adf;

)XXX";

  for (auto op : module.getOps<FuncOp>()) {
      if (op->getAttr("adf.kernel")){
        auto KName = op.getName();
        os << kernel_header;
        os << "void " << KName << "(";
        unsigned index = 0;
        for(auto arg: op.getArguments()){
          auto argType = arg.getType();
          if(auto memType = dyn_cast<MemRefType>(argType)){
            auto size = memType.getNumElements();
            os << "input_buffer<" << getTypeName(arg) << ", extents<" << size
            <<">> &in" << std::to_string(index++) << ", "; 
          }else{
            os << getTypeName(arg) << "in" << std::to_string(index++) << ", ";
          }
        }
        index = 0;
        auto outNum = op.getNumResults();
        if(outNum == 0)
          assert("The kernel declration without any outputs is wrong");
        for(auto outType: op.getResultTypes()){
          if(auto memType = dyn_cast<MemRefType>(outType)){
            auto size = memType.getNumElements();
            os << "output_buffer<" << getTypeName(outType) <<", extents<" <<size
            << ">> &out" << std::to_string(index++);
            if(index != outNum)
              os << ", ";
            else
              os << ");\n\n"; 
            
          }else{
            os << getTypeName(outType) << "out" << std::to_string(index++) << ", ";
            if(index != outNum)
              os << ", ";
            else
              os << ");\n\n"; 
          }
        }
        os << "#endif //__KERNEL_H__/\n";
      } 
  }
  
  return success();
}