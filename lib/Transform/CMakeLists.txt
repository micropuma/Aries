add_mlir_library(MLIRAriesPasses
    AriesTiling.cpp
    AriesFuncExtract.cpp
    AriesLoopSimplify.cpp
    AriesMemSubview.cpp
    AriesMemHoist.cpp
    AriesMemCopy.cpp
    AriesFuncUnroll.cpp
    AriesDependencyExtract.cpp
    AriesDMAToIO.cpp
    AriesADFGraphCreate.cpp
    AriesADFTest.cpp
    Passes.cpp
    Utils.cpp

    ADDITIONAL_HEADER_DIRS
    ${PROJECT_SOURCE_DIR}/include/aries

    DEPENDS
    MLIRAriesPassesIncGen

    LINK_LIBS PUBLIC
)