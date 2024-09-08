add_mlir_library(MLIRAriesPasses
    AriesTiling.cpp
    AriesFuncExtract.cpp
    AriesLoopSimplify.cpp
    AriesMemSubview.cpp
    AriesMemHoist.cpp
    AriesMemCopy.cpp
    AriesDependencyExtract.cpp
    AriesFuncUnroll.cpp
    AriesCorePlacement.cpp
    AriesLocalDataForward.cpp
    AriesKernelInterfaceCreate.cpp
    AriesDMAToIO.cpp
    AriesADFCellCreate.cpp
    AriesKernelSplit.cpp
    AriesGMIOMaterialize.cpp
    AriesPLIOMaterialize.cpp
    AriesPLDoubleBuffer.cpp
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