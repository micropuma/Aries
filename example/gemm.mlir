module {
  func.func @gemm(%arg0: memref<128x128xi32>, %arg1: memref<128x128xi32>, %arg2: memref<128x128xi32>) {
    affine.for %arg3 = 0 to 128 {
      affine.for %arg4 = 0 to 128 {
        affine.for %arg5 = 0 to 128 {
          %0 = affine.load %arg1[%arg3, %arg5] : memref<128x128xi32>
          %1 = affine.load %arg2[%arg5, %arg4] : memref<128x128xi32>
          %2 = arith.muli %0, %1 : i32
          %3 = affine.load %arg0[%arg3, %arg4] : memref<128x128xi32>
          %4 = arith.addi %3, %2 : i32
          affine.store %4, %arg0[%arg3, %arg4] : memref<128x128xi32>
        } {reduction}
      }
    }
    return
  }
}
