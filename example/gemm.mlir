module {
  func.func @gemm(%arg1: memref<2816x8192xi32>, %arg2: memref<8192x3072xi32>, %arg0: memref<2816x3072xi32>) {
    affine.for %arg3 = 0 to 2816 {
      affine.for %arg4 = 0 to 3072 {
        affine.for %arg5 = 0 to 8192 {
          %0 = affine.load %arg1[%arg3, %arg5] : memref<2816x8192xi32>
          %1 = affine.load %arg2[%arg5, %arg4] : memref<8192x3072xi32>
          %2 = arith.muli %0, %1 : i32
          %3 = affine.load %arg0[%arg3, %arg4] : memref<2816x3072xi32>
          %4 = arith.addi %3, %2 : i32
          affine.store %4, %arg0[%arg3, %arg4] : memref<2816x3072xi32>
        } {reduction}
      }
    }
    return
  }
}