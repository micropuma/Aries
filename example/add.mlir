module {
  func.func @add(%arg0: memref<64xf32>, %arg1: memref<64xf32>, 
                       %arg2: memref<64xf32>) {
    affine.for %arg3 = 0 to 64 {
        %0 = affine.load %arg1[%arg3] : memref<64xf32>
        %1 = affine.load %arg2[%arg3] : memref<64xf32>
        %2 = arith.addf %0, %1 : f32
        affine.store %2, %arg0[%arg3] : memref<64xf32>
    }
    return
  }
}

