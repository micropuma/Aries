module {
  func.func @gemm0(%A: memref<128x64xf32>, %B: memref<64x64xf32>, %C: memref<128x64xf32>){
    affine.for %arg3 = 0 to 128 {
      affine.for %arg4 = 0 to 64 {
        affine.for %arg5 = 0 to 64 {
          %0 = affine.load %A[%arg3, %arg5] : memref<128x64xf32>
          %1 = affine.load %B[%arg5, %arg4] : memref<64x64xf32>
          %2 = arith.mulf %0, %1 : f32
          %3 = affine.load %C[%arg3, %arg4] : memref<128x64xf32>
          %4 = arith.addf %3, %2 : f32
          affine.store %4, %C[%arg3, %arg4] : memref<128x64xf32>
        } {reduction}
      }
    }
    return
  }

  func.func @top(%A: memref<128x64xf32>, %B: memref<64x64xf32>, %C: memref<128x64xf32>, %D: memref<64x64xf32>, %E: memref<128x64xf32>)attributes {outArgs = [2 : i32, 4 : i32], top_func}{
    func.call @gemm0(%A, %B, %C) : (memref<128x64xf32>, memref<64x64xf32>, memref<128x64xf32>) -> ()
    func.call @gemm0(%C, %D, %E) : (memref<128x64xf32>, memref<64x64xf32>, memref<128x64xf32>) -> ()
    return
  }
}