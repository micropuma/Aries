set(LLVM_TARGET_DEFINITIONS ADFDialect.td)
mlir_tablegen(ADFDialect.h.inc -gen-dialect-decls -dialect=adf)
mlir_tablegen(ADFDialect.cpp.inc -gen-dialect-defs -dialect=adf)
add_public_tablegen_target(MLIRADFDialectIncGen)