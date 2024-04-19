PRO_PATH=`realpath $1`

rm -rf ${PRO_PATH}/build
mkdir ${PRO_PATH}/build && cd ${PRO_PATH}/build

cmake ../ \
    -GNinja \
    -DLLVM_DIR=$PWD/../externals/llvm-project/build/lib/cmake/llvm \
    -DMLIR_DIR=$PWD/../externals/llvm-project/build/lib/cmake/mlir \
    -DCMAKE_BUILD_TYPE=Debug \
    -DLLVM_ENABLE_ASSERTIONS=ON \
    -DLLVM_USE_LINKER=lld \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++

ninja

cd ${PRO_PATH}
