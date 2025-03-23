// -----// IR Dump After AriesKernelInterfaceCreate (aries-kernel-interface-create) //----- //
#map = affine_map<(d0, d1) -> (d0 * 352 + d1 * 1408)>
#map1 = affine_map<(d0, d1) -> (d0 * 128 + d1 * 128)>
#map2 = affine_map<(d0, d1) -> (d0 * 256 + d1 * 1536)>
#map3 = affine_map<(d0, d1) -> (d0 * 128 + d1 * 128 + 32)>
#map4 = affine_map<(d0, d1) -> (d0 * 256 + d1 * 1536 + 32)>
#map5 = affine_map<(d0, d1) -> (d0 * 352 + d1 * 1408 + 32)>
module {
  func.func @kernel_gemm0(%arg0: memref<32x32xf32, 2>, %arg1: memref<32x32xf32, 2>) -> memref<32x32xf32, 2> attributes {adf.kernel, edge_kernel} {
    %alloc = memref.alloc() : memref<32x32xf32, 2>
    affine.for %arg2 = 0 to 32 {
      affine.for %arg3 = 0 to 32 {
        %cst = arith.constant 0.000000e+00 : f32
        affine.store %cst, %alloc[%arg2, %arg3] : memref<32x32xf32, 2>
      }
    }
    affine.for %arg2 = 0 to 32 {
      affine.for %arg3 = 0 to 32 {
        affine.for %arg4 = 0 to 32 {
          %0 = affine.load %arg0[%arg2, %arg4] : memref<32x32xf32, 2>
          %1 = affine.load %arg1[%arg4, %arg3] : memref<32x32xf32, 2>
          %2 = arith.mulf %0, %1 : f32
          %3 = affine.load %alloc[%arg2, %arg3] : memref<32x32xf32, 2>
          %4 = arith.addf %3, %2 : f32
          affine.store %4, %alloc[%arg2, %arg3] : memref<32x32xf32, 2>
        }
      }
    }
    return %alloc : memref<32x32xf32, 2>
  }
  func.func @kernel_gemm(%arg0: memref<32x32xf32, 2>, %arg1: memref<32x32xf32, 2>, %arg2: memref<32x32xf32, 2>) -> memref<32x32xf32, 2> attributes {adf.kernel} {
    %alloc = memref.alloc() : memref<32x32xf32, 2>
    affine.for %arg3 = 0 to 32 {
      affine.for %arg4 = 0 to 32 {
        %0 = affine.load %arg2[%arg3, %arg4] : memref<32x32xf32, 2>
        affine.store %0, %alloc[%arg3, %arg4] : memref<32x32xf32, 2>
        affine.for %arg5 = 0 to 32 {
          %1 = affine.load %arg0[%arg3, %arg5] : memref<32x32xf32, 2>
          %2 = affine.load %arg1[%arg5, %arg4] : memref<32x32xf32, 2>
          %3 = arith.mulf %1, %2 : f32
          %4 = affine.load %alloc[%arg3, %arg4] : memref<32x32xf32, 2>
          %5 = arith.addf %4, %3 : f32
          affine.store %5, %alloc[%arg3, %arg4] : memref<32x32xf32, 2>
        }
      }
    }
    return %alloc : memref<32x32xf32, 2>
  }
  func.func @gemm(%arg0: memref<64x64xf32>, %arg1: memref<64x64xf32>, %arg2: memref<64x64xf32>) attributes {adf.func} {
    %c1 = arith.constant 1 : index
    %c32 = arith.constant 32 : index
    affine.for %arg3 = 0 to 1 {
      affine.for %arg4 = 0 to 1 {
        affine.for %arg5 = 0 to 1 {
          affine.for %arg6 = 0 to 1 {
            affine.for %arg7 = 0 to 1 {
              affine.for %arg8 = 0 to 1 {
                adf.cell @cell0 {
                  %0 = adf.buffer.create @L1_L1_A() : memref<32x32xf32, 2>
                  %1 = adf.buffer.create @L1_L1_B() : memref<32x32xf32, 2>
                  %2 = adf.buffer.create @L1_L1_C() : memref<32x32xf32, 2>
                  %3 = affine.apply #map(%arg6, %arg3)
                  %4 = affine.apply #map1(%arg8, %arg5)
                  adf.dma(%arg0[%3, %4] [%c32, %c32] [%c1, %c1], %0[] [] []) : (memref<64x64xf32>, memref<32x32xf32, 2>)
                  %5 = affine.apply #map2(%arg7, %arg4)
                  adf.dma(%arg1[%4, %5] [%c32, %c32] [%c1, %c1], %1[] [] []) : (memref<64x64xf32>, memref<32x32xf32, 2>)
                  %6 = func.call @kernel_gemm0(%0, %1) {adf.kernel, ivs = [0 : index, 0 : index, 0 : index], kernel = 0 : index, kernel_gemm0 = 0 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
                  %7 = adf.buffer.create @L1_L1_A_1() : memref<32x32xf32, 2>
                  %8 = adf.buffer.create @L1_L1_B_1() : memref<32x32xf32, 2>
                  %9 = adf.buffer.create @L1_L1_C_1() : memref<32x32xf32, 2>
                  %10 = affine.apply #map3(%arg8, %arg5)
                  adf.dma(%arg0[%3, %10] [%c32, %c32] [%c1, %c1], %7[] [] []) : (memref<64x64xf32>, memref<32x32xf32, 2>)
                  adf.dma(%arg1[%10, %5] [%c32, %c32] [%c1, %c1], %8[] [] []) : (memref<64x64xf32>, memref<32x32xf32, 2>)
                  adf.dma(%6[] [] [], %9[] [] []) : (memref<32x32xf32, 2>, memref<32x32xf32, 2>)
                  %11 = func.call @kernel_gemm(%7, %8, %9) {adf.kernel, ivs = [1 : index, 0 : index, 0 : index], kernel_gemm = 1 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
                  adf.dma(%11[] [] [], %arg2[%3, %5] [%c32, %c32] [%c1, %c1]) {write = 1 : index} : (memref<32x32xf32, 2>, memref<64x64xf32>)
                  %12 = adf.buffer.create @L1_L1_A_2() : memref<32x32xf32, 2>
                  %13 = adf.buffer.create @L1_L1_B_2() : memref<32x32xf32, 2>
                  %14 = adf.buffer.create @L1_L1_C_2() : memref<32x32xf32, 2>
                  adf.dma(%arg0[%3, %4] [%c32, %c32] [%c1, %c1], %12[] [] []) : (memref<64x64xf32>, memref<32x32xf32, 2>)
                  %15 = affine.apply #map4(%arg7, %arg4)
                  adf.dma(%arg1[%4, %15] [%c32, %c32] [%c1, %c1], %13[] [] []) : (memref<64x64xf32>, memref<32x32xf32, 2>)
                  %16 = func.call @kernel_gemm0(%12, %13) {adf.kernel, ivs = [0 : index, 1 : index, 0 : index], kernel = 0 : index, kernel_gemm0 = 2 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
                  %17 = adf.buffer.create @L1_L1_A_3() : memref<32x32xf32, 2>
                  %18 = adf.buffer.create @L1_L1_B_3() : memref<32x32xf32, 2>
                  %19 = adf.buffer.create @L1_L1_C_3() : memref<32x32xf32, 2>
                  adf.dma(%arg0[%3, %10] [%c32, %c32] [%c1, %c1], %17[] [] []) : (memref<64x64xf32>, memref<32x32xf32, 2>)
                  adf.dma(%arg1[%10, %15] [%c32, %c32] [%c1, %c1], %18[] [] []) : (memref<64x64xf32>, memref<32x32xf32, 2>)
                  adf.dma(%16[] [] [], %19[] [] []) : (memref<32x32xf32, 2>, memref<32x32xf32, 2>)
                  %20 = func.call @kernel_gemm(%17, %18, %19) {adf.kernel, ivs = [1 : index, 1 : index, 0 : index], kernel_gemm = 3 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
                  adf.dma(%20[] [] [], %arg2[%3, %15] [%c32, %c32] [%c1, %c1]) {write = 1 : index} : (memref<32x32xf32, 2>, memref<64x64xf32>)
                  %21 = adf.buffer.create @L1_L1_A_4() : memref<32x32xf32, 2>
                  %22 = adf.buffer.create @L1_L1_B_4() : memref<32x32xf32, 2>
                  %23 = adf.buffer.create @L1_L1_C_4() : memref<32x32xf32, 2>
                  %24 = affine.apply #map5(%arg6, %arg3)
                  adf.dma(%arg0[%24, %4] [%c32, %c32] [%c1, %c1], %21[] [] []) : (memref<64x64xf32>, memref<32x32xf32, 2>)
                  adf.dma(%arg1[%4, %5] [%c32, %c32] [%c1, %c1], %22[] [] []) : (memref<64x64xf32>, memref<32x32xf32, 2>)
                  %25 = func.call @kernel_gemm0(%21, %22) {adf.kernel, ivs = [0 : index, 0 : index, 1 : index], kernel = 0 : index, kernel_gemm0 = 4 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
                  %26 = adf.buffer.create @L1_L1_A_5() : memref<32x32xf32, 2>
                  %27 = adf.buffer.create @L1_L1_B_5() : memref<32x32xf32, 2>
                  %28 = adf.buffer.create @L1_L1_C_5() : memref<32x32xf32, 2>
                  adf.dma(%arg0[%24, %10] [%c32, %c32] [%c1, %c1], %26[] [] []) : (memref<64x64xf32>, memref<32x32xf32, 2>)
                  adf.dma(%arg1[%10, %5] [%c32, %c32] [%c1, %c1], %27[] [] []) : (memref<64x64xf32>, memref<32x32xf32, 2>)
                  adf.dma(%25[] [] [], %28[] [] []) : (memref<32x32xf32, 2>, memref<32x32xf32, 2>)
                  %29 = func.call @kernel_gemm(%26, %27, %28) {adf.kernel, ivs = [1 : index, 0 : index, 1 : index], kernel_gemm = 5 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
                  adf.dma(%29[] [] [], %arg2[%24, %5] [%c32, %c32] [%c1, %c1]) {write = 1 : index} : (memref<32x32xf32, 2>, memref<64x64xf32>)
                  %30 = adf.buffer.create @L1_L1_A_6() : memref<32x32xf32, 2>
                  %31 = adf.buffer.create @L1_L1_B_6() : memref<32x32xf32, 2>
                  %32 = adf.buffer.create @L1_L1_C_6() : memref<32x32xf32, 2>
                  adf.dma(%arg0[%24, %4] [%c32, %c32] [%c1, %c1], %30[] [] []) : (memref<64x64xf32>, memref<32x32xf32, 2>)
                  adf.dma(%arg1[%4, %15] [%c32, %c32] [%c1, %c1], %31[] [] []) : (memref<64x64xf32>, memref<32x32xf32, 2>)
                  %33 = func.call @kernel_gemm0(%30, %31) {adf.kernel, ivs = [0 : index, 1 : index, 1 : index], kernel = 0 : index, kernel_gemm0 = 6 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
                  %34 = adf.buffer.create @L1_L1_A_7() : memref<32x32xf32, 2>
                  %35 = adf.buffer.create @L1_L1_B_7() : memref<32x32xf32, 2>
                  %36 = adf.buffer.create @L1_L1_C_7() : memref<32x32xf32, 2>
                  adf.dma(%arg0[%24, %10] [%c32, %c32] [%c1, %c1], %34[] [] []) : (memref<64x64xf32>, memref<32x32xf32, 2>)
                  adf.dma(%arg1[%10, %15] [%c32, %c32] [%c1, %c1], %35[] [] []) : (memref<64x64xf32>, memref<32x32xf32, 2>)
                  adf.dma(%33[] [] [], %36[] [] []) : (memref<32x32xf32, 2>, memref<32x32xf32, 2>)
                  %37 = func.call @kernel_gemm(%34, %35, %36) {adf.kernel, ivs = [1 : index, 1 : index, 1 : index], kernel_gemm = 7 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
                  adf.dma(%37[] [] [], %arg2[%24, %15] [%c32, %c32] [%c1, %c1]) {write = 1 : index} : (memref<32x32xf32, 2>, memref<64x64xf32>)
                  adf.cell.end
                } {tripCount = [2 : index, 2 : index, 2 : index]}
              } {reduction}
            }
          }
        } {Array_Partition, reduction}
      }
    }
    return
  }
  func.func @top(%arg0: memref<64x64xf32>, %arg1: memref<64x64xf32>, %arg2: memref<64x64xf32>) attributes {outArgs = [2 : i32], top_func} {
    call @gemm(%arg0, %arg1, %arg2) : (memref<64x64xf32>, memref<64x64xf32>, memref<64x64xf32>) -> ()
    return
  }
  func.func private @gemm_host(memref<64x64xf32>, memref<64x64xf32>, memref<64x64xf32>) attributes {origin_func = "gemm"}
  func.func @top_host(%arg0: memref<64x64xf32>, %arg1: memref<64x64xf32>, %arg2: memref<64x64xf32>) attributes {origin_func = "top", outArgs = [2 : i32], top_host} {
    call @gemm_host(%arg0, %arg1, %arg2) {origin_func = "gemm"} : (memref<64x64xf32>, memref<64x64xf32>, memref<64x64xf32>) -> ()
    return
  }
}
