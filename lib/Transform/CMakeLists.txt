add_mlir_library(MLIRAriesPasses
    AriesTiling.cpp
    AriesFuncExtract.cpp
    AriesLoopSimplify.cpp
    AriesMemSubview.cpp
    AriesFuncUnroll.cpp
    Passes.cpp
    Utils.cpp

    ADDITIONAL_HEADER_DIRS
    ${PROJECT_SOURCE_DIR}/include/aries

    DEPENDS
    MLIRAriesPassesIncGen

    LINK_LIBS PUBLIC
)