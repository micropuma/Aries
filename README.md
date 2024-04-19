# Aries
## Installation Guide


### 1. Download Aries & Submodules
```sh
$ git clone --recursive https://github.com/JinmingZhuang/Aries_Local.git
$ cd Aries_Local
```

### 2. Build LLVM & MLIR Projects
```sh
$ cd externals/llvm-project
$ mkdir build && cd build

$ cmake ../llvm \
    -GNinja \
    -DLLVM_BUILD_EXAMPLES=OFF \
    -DLLVM_ENABLE_PROJECTS=mlir \
    -DLLVM_TARGETS_TO_BUILD="host" \
    -DLLVM_ENABLE_ASSERTIONS=ON \
    -DCMAKE_BUILD_TYPE=DEBUG \
    -DLLVM_USE_LINKER=lld \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++

$ ninja && ninja check-mlir
```

### 3. Build Aries Project
```sh
$ cd Aries
$ mkdir build && cd build

$ cmake ../ \
    -GNinja \
    -DLLVM_DIR=$PWD/../externals/llvm-project/build/lib/cmake/llvm \
    -DMLIR_DIR=$PWD/../externals/llvm-project/build/lib/cmake/mlir \
    -DCMAKE_BUILD_TYPE=Debug \
    -DLLVM_ENABLE_ASSERTIONS=ON \
    -DLLVM_USE_LINKER=lld \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++
    
```
