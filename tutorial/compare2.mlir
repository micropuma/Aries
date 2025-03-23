// -----// IR Dump After AriesDMAToIO (aries-dma-to-io) //----- //
#map = affine_map<(d0, d1) -> (d0 * 352 + d1 * 1408)>
#map1 = affine_map<(d0, d1) -> (d0 * 32 + d1 * 32)>
#map2 = affine_map<(d0, d1) -> (d0 * 128 + d1 * 128)>
#map3 = affine_map<(d0, d1) -> (d0 * 64 + d1 * 384)>
#map4 = affine_map<(d0, d1) -> (d0 * 32 + d1 * 32 + 8)>
#map5 = affine_map<(d0, d1) -> (d0 * 128 + d1 * 128 + 32)>
#map6 = affine_map<(d0, d1) -> (d0 * 64 + d1 * 384 + 8)>
#map7 = affine_map<(d0, d1) -> (d0 * 352 + d1 * 1408 + 32)>
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
  func.func @gemm(%arg0: memref<64x16xi128>, %arg1: memref<64x16xi128>, %arg2: memref<64x16xi128>) attributes {adf.func, plio = true} {
    %c8 = arith.constant 8 : index
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
                  %2 = affine.apply #map(%arg6, %arg3)
                  %3 = affine.apply #map1(%arg8, %arg5)
                  %4 = affine.apply #map2(%arg8, %arg5)
                  %5 = adf.graph.io(PLIO) : !adf.plio<In, 128>
                  adf.config.plio(%5, 250) : <In, 128>
                  adf.io.push(%arg0[%2, %3] [%c32, %c8] [%c1, %c1], %5) {type = f32} : (memref<64x16xi128>, !adf.plio<In, 128>)
                  adf.connect(%5, %0) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
                  %6 = affine.apply #map3(%arg7, %arg4)
                  %7 = adf.graph.io(PLIO) : !adf.plio<In, 128>
                  adf.config.plio(%7, 250) : <In, 128>
                  adf.io.push(%arg1[%4, %6] [%c32, %c8] [%c1, %c1], %7) {type = f32} : (memref<64x16xi128>, !adf.plio<In, 128>)
                  adf.connect(%7, %1) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
                  %8 = func.call @kernel_gemm0(%0, %1) {adf.kernel, ivs = [0 : index, 0 : index, 0 : index], kernel = 0 : index, kernel_gemm0 = 0 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
                  %9 = adf.buffer.create @L1_L1_A_1() : memref<32x32xf32, 2>
                  %10 = adf.buffer.create @L1_L1_B_1() : memref<32x32xf32, 2>
                  %11 = adf.buffer.create @L1_L1_C_1() : memref<32x32xf32, 2>
                  %12 = affine.apply #map4(%arg8, %arg5)
                  %13 = affine.apply #map5(%arg8, %arg5)
                  %14 = adf.graph.io(PLIO) : !adf.plio<In, 128>
                  adf.config.plio(%14, 250) : <In, 128>
                  adf.io.push(%arg0[%2, %12] [%c32, %c8] [%c1, %c1], %14) {type = f32} : (memref<64x16xi128>, !adf.plio<In, 128>)
                  adf.connect(%14, %9) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
                  %15 = adf.graph.io(PLIO) : !adf.plio<In, 128>
                  adf.config.plio(%15, 250) : <In, 128>
                  adf.io.push(%arg1[%13, %6] [%c32, %c8] [%c1, %c1], %15) {type = f32} : (memref<64x16xi128>, !adf.plio<In, 128>)
                  adf.connect(%15, %10) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
                  adf.connect(%8, %11) : (memref<32x32xf32, 2>, memref<32x32xf32, 2>)
                  %16 = func.call @kernel_gemm(%9, %10, %11) {adf.kernel, ivs = [1 : index, 0 : index, 0 : index], kernel_gemm = 1 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
                  %17 = adf.graph.io(PLIO) : !adf.plio<Out, 128>
                  adf.config.plio(%17, 250) : <Out, 128>
                  adf.connect(%16, %17) {write = 1 : index} : (memref<32x32xf32, 2>, !adf.plio<Out, 128>)
                  adf.io.pop(%17, %arg2[%2, %6] [%c32, %c8] [%c1, %c1]) {type = f32} : (!adf.plio<Out, 128>, memref<64x16xi128>)
                  %18 = adf.buffer.create @L1_L1_A_2() : memref<32x32xf32, 2>
                  %19 = adf.buffer.create @L1_L1_B_2() : memref<32x32xf32, 2>
                  %20 = adf.graph.io(PLIO) : !adf.plio<In, 128>
                  adf.config.plio(%5, 250) : <In, 128>
                  adf.connect(%5, %18) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
                  %21 = affine.apply #map6(%arg7, %arg4)
                  %22 = adf.graph.io(PLIO) : !adf.plio<In, 128>
                  adf.config.plio(%22, 250) : <In, 128>
                  adf.io.push(%arg1[%4, %21] [%c32, %c8] [%c1, %c1], %22) {type = f32} : (memref<64x16xi128>, !adf.plio<In, 128>)
                  adf.connect(%22, %19) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
                  %23 = func.call @kernel_gemm0(%18, %19) {adf.kernel, ivs = [0 : index, 1 : index, 0 : index], kernel = 0 : index, kernel_gemm0 = 2 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
                  %24 = adf.buffer.create @L1_L1_A_3() : memref<32x32xf32, 2>
                  %25 = adf.buffer.create @L1_L1_B_3() : memref<32x32xf32, 2>
                  %26 = adf.buffer.create @L1_L1_C_3() : memref<32x32xf32, 2>
                  %27 = adf.graph.io(PLIO) : !adf.plio<In, 128>
                  adf.config.plio(%14, 250) : <In, 128>
                  adf.connect(%14, %24) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
                  %28 = adf.graph.io(PLIO) : !adf.plio<In, 128>
                  adf.config.plio(%28, 250) : <In, 128>
                  adf.io.push(%arg1[%13, %21] [%c32, %c8] [%c1, %c1], %28) {type = f32} : (memref<64x16xi128>, !adf.plio<In, 128>)
                  adf.connect(%28, %25) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
                  adf.connect(%23, %26) : (memref<32x32xf32, 2>, memref<32x32xf32, 2>)
                  %29 = func.call @kernel_gemm(%24, %25, %26) {adf.kernel, ivs = [1 : index, 1 : index, 0 : index], kernel_gemm = 3 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
                  %30 = adf.graph.io(PLIO) : !adf.plio<Out, 128>
                  adf.config.plio(%30, 250) : <Out, 128>
                  adf.connect(%29, %30) {write = 1 : index} : (memref<32x32xf32, 2>, !adf.plio<Out, 128>)
                  adf.io.pop(%30, %arg2[%2, %21] [%c32, %c8] [%c1, %c1]) {type = f32} : (!adf.plio<Out, 128>, memref<64x16xi128>)
                  %31 = adf.buffer.create @L1_L1_A_4() : memref<32x32xf32, 2>
                  %32 = adf.buffer.create @L1_L1_B_4() : memref<32x32xf32, 2>
                  %33 = affine.apply #map7(%arg6, %arg3)
                  %34 = adf.graph.io(PLIO) : !adf.plio<In, 128>
                  adf.config.plio(%34, 250) : <In, 128>
                  adf.io.push(%arg0[%33, %3] [%c32, %c8] [%c1, %c1], %34) {type = f32} : (memref<64x16xi128>, !adf.plio<In, 128>)
                  adf.connect(%34, %31) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
                  %35 = adf.graph.io(PLIO) : !adf.plio<In, 128>
                  adf.config.plio(%7, 250) : <In, 128>
                  adf.connect(%7, %32) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
                  %36 = func.call @kernel_gemm0(%31, %32) {adf.kernel, ivs = [0 : index, 0 : index, 1 : index], kernel = 0 : index, kernel_gemm0 = 4 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
                  %37 = adf.buffer.create @L1_L1_A_5() : memref<32x32xf32, 2>
                  %38 = adf.buffer.create @L1_L1_B_5() : memref<32x32xf32, 2>
                  %39 = adf.buffer.create @L1_L1_C_5() : memref<32x32xf32, 2>
                  %40 = adf.graph.io(PLIO) : !adf.plio<In, 128>
                  adf.config.plio(%40, 250) : <In, 128>
                  adf.io.push(%arg0[%33, %12] [%c32, %c8] [%c1, %c1], %40) {type = f32} : (memref<64x16xi128>, !adf.plio<In, 128>)
                  adf.connect(%40, %37) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
                  %41 = adf.graph.io(PLIO) : !adf.plio<In, 128>
                  adf.config.plio(%15, 250) : <In, 128>
                  adf.connect(%15, %38) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
                  adf.connect(%36, %39) : (memref<32x32xf32, 2>, memref<32x32xf32, 2>)
                  %42 = func.call @kernel_gemm(%37, %38, %39) {adf.kernel, ivs = [1 : index, 0 : index, 1 : index], kernel_gemm = 5 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
                  %43 = adf.graph.io(PLIO) : !adf.plio<Out, 128>
                  adf.config.plio(%43, 250) : <Out, 128>
                  adf.connect(%42, %43) {write = 1 : index} : (memref<32x32xf32, 2>, !adf.plio<Out, 128>)
                  adf.io.pop(%43, %arg2[%33, %6] [%c32, %c8] [%c1, %c1]) {type = f32} : (!adf.plio<Out, 128>, memref<64x16xi128>)
                  %44 = adf.buffer.create @L1_L1_A_6() : memref<32x32xf32, 2>
                  %45 = adf.buffer.create @L1_L1_B_6() : memref<32x32xf32, 2>
                  %46 = adf.graph.io(PLIO) : !adf.plio<In, 128>
                  adf.config.plio(%34, 250) : <In, 128>
                  adf.connect(%34, %44) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
                  %47 = adf.graph.io(PLIO) : !adf.plio<In, 128>
                  adf.config.plio(%22, 250) : <In, 128>
                  adf.connect(%22, %45) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
                  %48 = func.call @kernel_gemm0(%44, %45) {adf.kernel, ivs = [0 : index, 1 : index, 1 : index], kernel = 0 : index, kernel_gemm0 = 6 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
                  %49 = adf.buffer.create @L1_L1_A_7() : memref<32x32xf32, 2>
                  %50 = adf.buffer.create @L1_L1_B_7() : memref<32x32xf32, 2>
                  %51 = adf.buffer.create @L1_L1_C_7() : memref<32x32xf32, 2>
                  %52 = adf.graph.io(PLIO) : !adf.plio<In, 128>
                  adf.config.plio(%40, 250) : <In, 128>
                  adf.connect(%40, %49) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
                  %53 = adf.graph.io(PLIO) : !adf.plio<In, 128>
                  adf.config.plio(%28, 250) : <In, 128>
                  adf.connect(%28, %50) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
                  adf.connect(%48, %51) : (memref<32x32xf32, 2>, memref<32x32xf32, 2>)
                  %54 = func.call @kernel_gemm(%49, %50, %51) {adf.kernel, ivs = [1 : index, 1 : index, 1 : index], kernel_gemm = 7 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
                  %55 = adf.graph.io(PLIO) : !adf.plio<Out, 128>
                  adf.config.plio(%55, 250) : <Out, 128>
                  adf.connect(%54, %55) {write = 1 : index} : (memref<32x32xf32, 2>, !adf.plio<Out, 128>)
                  adf.io.pop(%55, %arg2[%33, %21] [%c32, %c8] [%c1, %c1]) {type = f32} : (!adf.plio<Out, 128>, memref<64x16xi128>)
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
  func.func @top(%arg0: memref<64x16xi128>, %arg1: memref<64x16xi128>, %arg2: memref<64x16xi128>) attributes {outArgs = [2 : i32], top_func} {
    call @gemm(%arg0, %arg1, %arg2) : (memref<64x16xi128>, memref<64x16xi128>, memref<64x16xi128>) -> ()
    return
  }
  func.func private @gemm_host(memref<64x64xf32>, memref<64x64xf32>, memref<64x64xf32>) attributes {origin_func = "gemm"}
  func.func @top_host(%arg0: memref<64x64xf32>, %arg1: memref<64x64xf32>, %arg2: memref<64x64xf32>) attributes {origin_func = "top", outArgs = [2 : i32], top_host} {
    call @gemm_host(%arg0, %arg1, %arg2) {origin_func = "gemm"} : (memref<64x64xf32>, memref<64x64xf32>, memref<64x64xf32>) -> ()
    return
  }
}
