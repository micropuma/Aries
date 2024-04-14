add_mlir_library(MLIRAriesPasses
    AriesTiling.cpp
    Passes.cpp

    ADDITIONAL_HEADER_DIRS
    ${PROJECT_SOURCE_DIR}/include/aries

    DEPENDS
    MLIRAriesPassesIncGen

    LINK_LIBS PUBLIC
)