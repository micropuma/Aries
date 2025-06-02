#map0 = affine_map<(d0) -> (32*d0)>
module {
  func.func @relu_tile_1d(%X: memref<64xf32>, %Y: memref<64xf32>) attributes {adf.func} {
    affine.for %i = 0 to 2 {
      affine.for %_ = 0 to 1 {
        %L1_X = adf.buffer.create @L1_L1_X() : memref<32xf32, 2>
        %L1_Y = adf.buffer.create @L1_L1_Y() : memref<32xf32, 2>
        %v0 = affine.apply #map0(%i)
        %cti_size = arith.constant 32 : index
        %cti_stride = arith.constant 1 : index
        adf.dma(%X[%v0] [%cti_size] [%cti_stride], %L1_X[] [] []) : (memref<64xf32> , memref<32xf32, 2>)
        affine.for %i_1 = 0 to 32 {
        }
        %v1 = affine.apply #map0(%i_1)
        %cti_size_1 = arith.constant 32 : index
        %cti_stride_1 = arith.constant 1 : index
        adf.dma(%L1_Y[] [] [], %Y[%v1] [%cti_size_1] [%cti_stride_1]) : (memref<32xf32, 2> , memref<64xf32>)
      }
    }{reduction}
    return
  } 
  func.func @top_1d(%X: memref<64xf32>, %Y: memref<64xf32>) attributes {top_func} {
    func.call @relu_tile_1d(%X, %Y) : (memref<64xf32>, memref<64xf32>) -> ()
    return
  } 
}
