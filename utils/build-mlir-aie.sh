PRO_PATH=$PWD
LLVM_DIR=`realpath ${1:-"$PWD/externals/llvm-project"}`
AIE_DIR=`realpath ${2:-"$PWD/externals/mlir-aie"}`
CMAKE=`realpath ${3:-"$PWD/aries/bin/cmake"}`
CMAKEMODULES_DIR=`realpath ${3:-"$PWD/externals/mlir-aie/cmake/modulesXilinx"}`

BUILD_DIR=build
INSTALL_DIR=install
BUILD_TARGET=x86_64
LLVM_ENABLE_RTTI=${LLVM_ENABLE_RTTI:OFF}

mkdir -p $AIE_DIR/$BUILD_DIR
mkdir -p $AIE_DIR/$INSTALL_DIR
cd $AIE_DIR/$BUILD_DIR

$CMAKE -G Ninja \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DLLVM_USE_LINKER=lld \
    -DLLVM_DIR=${LLVM_DIR}/build/lib/cmake/llvm \
    -DMLIR_DIR=${LLVM_DIR}/build/lib/cmake/mlir \
    -DCMAKE_MODULE_PATH=${CMAKEMODULES_DIR}/ \
    -DCMAKE_INSTALL_PREFIX="../${INSTALL_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PLATFORM_NO_VERSIONED_SONAME=ON \
    -DCMAKE_VISIBILITY_INLINES_HIDDEN=ON \
    -DCMAKE_C_VISIBILITY_PRESET=hidden \
    -DCMAKE_CXX_VISIBILITY_PRESET=hidden \
    -DLLVM_ENABLE_ASSERTIONS=ON \
    -DLLVM_ENABLE_RTTI=$LLVM_ENABLE_RTTI \
    -DAIE_RUNTIME_TARGETS=$BUILD_TARGET \
    -DAIE_ENABLE_PYTHON_PASSES=OFF \
    -DAIE_RUNTIME_TEST_TARGET=$BUILD_TARGET \
    .. |& tee cmake.log

ninja |& tee ninja.log
ninja install |& tee ninja-install.log

cd ${PRO_PATH}