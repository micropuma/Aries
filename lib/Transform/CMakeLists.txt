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
    AriesBroadcastToForwarding.cpp
    AriesDMAToIO.cpp
    AriesADFCellCreate.cpp
    AriesIOPlacement.cpp
    AriesKernelSplit.cpp
    AriesGMIOMaterialize.cpp
    AriesPLIOMaterialize.cpp
    AriesAXIPacking.cpp
    AriesPLDataflow.cpp
    AriesPLDoubleBuffer.cpp
    AriesBurstDetection.cpp
    AriesFuncEliminate.cpp
    AriesFileSplit.cpp
    Passes.cpp
    Utils.cpp

    ADDITIONAL_HEADER_DIRS
    ${PROJECT_SOURCE_DIR}/include/aries

    DEPENDS
    MLIRAriesPassesIncGen

    LINK_LIBS PUBLIC
)