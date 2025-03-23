#map0 = affine_map<(d0) -> (32*d0)>
#map1 = affine_map<(d0) -> (32*d0)>
#map2 = affine_map<(d0) -> (32*d0)>
module {
  func.func @kernel_gemm(%TileA: memref<32x32xf32, 2>, %TileB: memref<32x32xf32, 2>, %TileC: memref<32x32xf32, 2>) attributes {adf.kernel} {
    affine.for %i0 = 0 to 32 {
      affine.for %j0 = 0 to 32 {
        affine.for %k0 = 0 to 32 {
          ... 
          %v2 = arith.mulf %v0, %v1 : f32
          ...
        }
      }
    }
    return
  } 
  func.func @gemm(%A: memref<64x64xf32>, %B: memref<64x64xf32>, %C: memref<64x64xf32>) attributes {adf.func} {
    affine.for %i = 0 to 2 {
      affine.for %j = 0 to 2 {
        affine.for %k = 0 to 2 {
          %L1_A = adf.buffer.create @L1_L1_A() : memref<32x32xf32, 2>
          %L1_B = adf.buffer.create @L1_L1_B() : memref<32x32xf32, 2>
          %L1_C = adf.buffer.create @L1_L1_C() : memref<32x32xf32, 2>
          ...
          adf.dma(%A[%v0 ,%v1] [%cti_size ,%ctk_size] [%cti_stride ,%ctk_stride], %L1_A[] [] []) : (memref<64x64xf32> , memref<32x32xf32, 2>)
          ...
          adf.dma(%B[%v2 ,%v3] [%ctk_size_1 ,%ctj_size] [%ctk_stride_1 ,%ctj_stride], %L1_B[] [] []) : (memref<64x64xf32> , memref<32x32xf32, 2>)
          ...
          adf.dma(%C[%v4 ,%v5] [%cti_size_1 ,%ctj_size_1] [%cti_stride_1 ,%ctj_stride_1], %L1_C[] [] []) : (memref<64x64xf32> , memref<32x32xf32, 2>)
          func.call @kernel_gemm(%L1_A, %L1_B, %L1_C) : (memref<32x32xf32, 2>, memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> ()
          ...
          adf.dma(%L1_C[] [] [], %C[%v6 ,%v7] [%cti_size_2 ,%ctj_size_2] [%cti_stride_2 ,%ctj_stride_2]) : (memref<32x32xf32, 2> , memref<64x64xf32>)
        }{reduction}
      }
    }
    return
  } 
  func.func @top(%A: memref<64x64xf32>, %B: memref<64x64xf32>, %C: memref<64x64xf32>) attributes {top_func} {
    func.call @gemm(%A, %B, %C) : (memref<64x64xf32>, memref<64x64xf32>, memref<64x64xf32>) -> ()
    return
  } 
}
