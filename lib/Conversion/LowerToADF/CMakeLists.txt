add_mlir_library(LowerToADF
  LowerToADF.cpp

  ADDITIONAL_HEADER_DIRS
  ${PROJECT_SOURCE_DIR}/include/aries/Conversion
  
  DEPENDS
  LowerToADFPassIncGen

  LINK_COMPONENTS
  Core

  LINK_LIBS PUBLIC
  MLIRADF
  MLIRIR
  MLIRPass
  MLIRTransforms
  )