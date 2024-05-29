set(LLVM_TARGET_DEFINITIONS LowerToADF.td)
mlir_tablegen(LowerToADF.h.inc -gen-pass-decls)
add_public_tablegen_target(LowerToADFPassIncGen)