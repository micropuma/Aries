module {
  func.func @gemm0(%A: memref<?x?xf32>, %B: memref<?x?xf32>, %C: memref<?x?xf32>, %Size_I: index, %Size_K: index, %Size_J: index) {
    affine.for %arg3 = 0 to %Size_I {
      affine.for %arg4 = 0 to %Size_J {
        affine.for %arg5 = 0 to %Size_K {
          %0 = affine.load %A[%arg3, %arg5] : memref<?x?xf32>
          %1 = affine.load %B[%arg5, %arg4] : memref<?x?xf32>
          %2 = arith.mulf %0, %1 : f32
          %3 = affine.load %C[%arg3, %arg4] : memref<?x?xf32>
          %4 = arith.addf %3, %2 : f32
          affine.store %4, %C[%arg3, %arg4] : memref<?x?xf32>
        } {reduction}
      }
    }
    return
  }
  func.func @top(%A: memref<?x?xf32>, %B: memref<?x?xf32>, %C: memref<?x?xf32>, %D: memref<?x?xf32>, %E: memref<?x?xf32>)attributes {top_func}{
    %c32 = arith.constant 32 : index
    %c64 = arith.constant 64 : index
    %c128 = arith.constant 128 : index
    func.call @gemm0(%A, %B, %C, %c128, %c64, %c32) : (memref<?x?xf32>, memref<?x?xf32>, memref<?x?xf32>, index, index, index) -> ()
    func.call @gemm0(%C, %D, %E, %c128, %c32, %c64) : (memref<?x?xf32>, memref<?x?xf32>, memref<?x?xf32>, index, index, index) -> ()
    return
  }
}