#include "mlir/InitAllDialects.h"
#include "mlir/InitAllPasses.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassRegistry.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"
#include "aries/Transform/Passes.h"
#include "aries/Dialect/ADF/ADFDialect.h"


int main(int argc, char **argv) {
    mlir::DialectRegistry registry;
    mlir::registerAllDialects(registry);
    registry.insert<mlir::aries::adf::ADFDialect>();
    mlir::registerAllPasses();

    mlir::aries::registerAriesPasses();

    return mlir::asMainReturnCode(
        mlir::MlirOptMain(argc, argv, "ARIES modular optimizer Driver", registry));
}


