add_mlir_library(MLIRAriesLowerToADF
  Passes.cpp
  AriesLowerToADF.cpp

  ADDITIONAL_HEADER_DIRS
  ${PROJECT_SOURCE_DIR}/include/aries/Conversion
  
  DEPENDS
  MLIRAriesLowerToADFPassIncGen

  LINK_COMPONENTS
  Core

  LINK_LIBS PUBLIC
  MLIRADF
  MLIRIR
  MLIRPass
  MLIRTransforms
  )