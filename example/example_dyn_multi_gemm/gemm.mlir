module {
  func.func @gemm0(%A: memref<?x?xf32>, %B: memref<?x?xf32>, %C: memref<?x?xf32>, %Size_I: index, %Size_K: index, %Size_J: index, %Size_A0: index, %Size_B0: index, %Size_C0: index)attributes {meta_data = [[0, 6], [1, 7], [2, 8]]} {
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
  func.func @top(%A: memref<128x32xf32>, %B: memref<32x64xf32>, %C: memref<128x64xf32>, %D: memref<64x32xf32>, %E: memref<128x32xf32>)attributes {top_func}{
    %A_cast = memref.cast %A : memref<128x32xf32> to memref<?x?xf32>
    %B_cast = memref.cast %B : memref<32x64xf32> to memref<?x?xf32>
    %C_cast = memref.cast %C : memref<128x64xf32> to memref<?x?xf32>
    %D_cast = memref.cast %D : memref<64x32xf32> to memref<?x?xf32>
    %E_cast = memref.cast %E : memref<128x32xf32> to memref<?x?xf32>
    %c32 = arith.constant 32 : index
    %c64 = arith.constant 64 : index
    %c128 = arith.constant 128 : index
    func.call @gemm0(%A_cast, %B_cast, %C_cast, %c128, %c64, %c32, %c32, %c64, %c64) : (memref<?x?xf32>, memref<?x?xf32>, memref<?x?xf32>, index, index, index, index, index, index) -> ()
    func.call @gemm0(%C_cast, %D_cast, %E_cast, %c128, %c32, %c64, %c64, %c32, %c32) : (memref<?x?xf32>, memref<?x?xf32>, memref<?x?xf32>, index, index, index, index, index, index) -> ()
    return
  }
}