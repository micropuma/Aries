add_mlir_dialect_library(MLIRADF
  ADFDialect.cpp

  ADDITIONAL_HEADER_DIRS
  ${PROJECT_SOURCE_DIR}/include/aries

  DEPENDS
  MLIRADFDialectIncGen

	LINK_LIBS PUBLIC
	)