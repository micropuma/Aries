add_mlir_library(MLIRAriesPasses
    AriesTiling.cpp
    AriesFuncExtract.cpp
    AriesLoopSimplify.cpp
    AriesMemSubview.cpp
    AriesMemHoist.cpp
    AriesMemCopy.cpp
    AriesDependencyExtract.cpp
    AriesFuncUnroll.cpp
    AriesKernelInterfaceCreate.cpp
    AriesLocalDataForward.cpp
    AriesADFCellCreate.cpp
    AriesKernelSplit.cpp
    AriesGMIOMaterialize.cpp
    AriesPLIOMaterialize.cpp
    AriesADFTest.cpp
    AriesFileSplit.cpp
    Passes.cpp
    Utils.cpp

    ADDITIONAL_HEADER_DIRS
    ${PROJECT_SOURCE_DIR}/include/aries

    DEPENDS
    MLIRAriesPassesIncGen

    LINK_LIBS PUBLIC
)