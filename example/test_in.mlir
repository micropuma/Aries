module {
  func.func @kernel_gemm(%arg0: memref<32x32xf32>, %arg1: memref<32x32xf32>) -> memref<32x32xf32>{
    %alloc_0 = memref.alloc() : memref<32x32xf32>
    return %alloc_0 : memref<32x32xf32>
  }
  func.func @gemm(%arg0: memref<64x64xf32>, %arg1: memref<64x64xf32>, %arg2: memref<64x64xf32>) attributes {top_func} {
    %plio_in = adf.plio.create() : !adf.plio<In, 128>
    %gmio_in = adf.gmio.create() : !adf.gmio<In, 128>
    adf.graph(%plio_in, %gmio_in){
      %alloc_0 = memref.alloc() : memref<32x32xf32>
      %alloc_1 = memref.alloc() : memref<32x32xf32>
      %gmio_out = adf.gmio.create() : !adf.gmio<Out, 128>
      %out0 = adf.kernel @kernel_gemm(%alloc_0, %alloc_1) : (memref<32x32xf32>, memref<32x32xf32>) -> memref<32x32xf32>
      adf.return %gmio_out : !adf.gmio<Out, 128>
    } : (!adf.plio<In, 128>, !adf.gmio<In, 128>)
    return
  }
}

