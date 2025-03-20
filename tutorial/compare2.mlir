// -----// IR Dump After AriesPLIOMaterialize (aries-plio-materialize) //----- //
module {
  func.func @kernel_gemm0(%arg0: memref<32x32xf32, 2>, %arg1: memref<32x32xf32, 2>) -> memref<32x32xf32, 2> attributes {adf.kernel, edge_kernel} {
    %cst = arith.constant 0.000000e+00 : f32
    %alloc = memref.alloc() : memref<32x32xf32, 2>
    affine.for %arg2 = 0 to 32 {
      affine.for %arg3 = 0 to 32 {
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
  func.func @adf_cell0(%arg0: !adf.plio<In, 128>, %arg1: !adf.plio<In, 128>, %arg2: !adf.plio<In, 128>, %arg3: !adf.plio<In, 128>, %arg4: !adf.plio<Out, 128>, %arg5: !adf.plio<In, 128>, %arg6: !adf.plio<In, 128>, %arg7: !adf.plio<Out, 128>, %arg8: !adf.plio<In, 128>, %arg9: !adf.plio<In, 128>, %arg10: !adf.plio<Out, 128>, %arg11: !adf.plio<Out, 128>) attributes {adf.cell, tripCount = [2 : index, 2 : index, 2 : index]} {
    %c4 = arith.constant 4 : index
    %c26 = arith.constant 26 : index
    %c12288 = arith.constant 12288 : index
    %c4096 = arith.constant 4096 : index
    %c2 = arith.constant 2 : index
    %c3 = arith.constant 3 : index
    %c24576 = arith.constant 24576 : index
    %c16384 = arith.constant 16384 : index
    %c25 = arith.constant 25 : index
    %c23 = arith.constant 23 : index
    %c8192 = arith.constant 8192 : index
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c24 = arith.constant 24 : index
    adf.config.plio(%arg4, 250) {"col, chl" = [25 : index, 4 : index]} : <Out, 128>
    adf.config.plio(%arg0, 250) {"col, chl" = [24 : index, 4 : index]} : <In, 128>
    adf.config.plio(%arg2, 250) {"col, chl" = [25 : index, 4 : index]} : <In, 128>
    adf.config.plio(%arg7, 250) {"col, chl" = [25 : index, 2 : index]} : <Out, 128>
    adf.config.plio(%arg1, 250) {"col, chl" = [24 : index, 2 : index]} : <In, 128>
    adf.config.plio(%arg3, 250) {"col, chl" = [25 : index, 2 : index]} : <In, 128>
    adf.config.plio(%arg10, 250) {"col, chl" = [25 : index, 0 : index]} : <Out, 128>
    adf.config.plio(%arg8, 250) {"col, chl" = [24 : index, 0 : index]} : <In, 128>
    adf.config.plio(%arg5, 250) {"col, chl" = [23 : index, 4 : index]} : <In, 128>
    adf.config.plio(%arg9, 250) {"col, chl" = [25 : index, 0 : index]} : <In, 128>
    adf.config.plio(%arg6, 250) {"col, chl" = [26 : index, 4 : index]} : <In, 128>
    adf.config.plio(%arg11, 250) {"col, chl" = [24 : index, 4 : index]} : <Out, 128>
    %0 = adf.buffer.create @L1_L1_A() : memref<32x32xf32, 2>
    %1 = adf.buffer.create @L1_L1_B() : memref<32x32xf32, 2>
    adf.connect(%arg0, %0) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%arg1, %1) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    %2 = call @kernel_gemm0(%0, %1) {adf.kernel, "col, row" = [24 : index, 0 : index], ivs = [0 : index, 0 : index, 0 : index], kernel = 0 : index, kernel_gemm0 = 0 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
    adf.buffer.location(%2, %c24, %c0, %c0, %c8192) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%1, %c23, %c0, %c0, %c8192) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%0, %c24, %c1, %c0, %c8192) : (memref<32x32xf32, 2>, index, index, index, index)
    %3 = adf.buffer.create @L1_L1_A_1() : memref<32x32xf32, 2>
    %4 = adf.buffer.create @L1_L1_B_1() : memref<32x32xf32, 2>
    %5 = adf.buffer.create @L1_L1_C_1() : memref<32x32xf32, 2>
    adf.connect(%arg2, %3) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%arg3, %4) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%2, %5) : (memref<32x32xf32, 2>, memref<32x32xf32, 2>)
    %6 = call @kernel_gemm(%3, %4, %5) {adf.kernel, "col, row" = [25 : index, 0 : index], ivs = [1 : index, 0 : index, 0 : index], kernel_gemm = 1 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
    adf.buffer.location(%6, %c25, %c0, %c0, %c8192) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%4, %c24, %c0, %c16384, %c24576) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%3, %c25, %c1, %c0, %c8192) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.connect(%6, %arg4) {write = 1 : index} : (memref<32x32xf32, 2>, !adf.plio<Out, 128>)
    %7 = adf.buffer.create @L1_L1_A_2() : memref<32x32xf32, 2>
    %8 = adf.buffer.create @L1_L1_B_2() : memref<32x32xf32, 2>
    adf.connect(%arg0, %7) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%arg5, %8) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    %9 = call @kernel_gemm0(%7, %8) {adf.kernel, "col, row" = [24 : index, 2 : index], ivs = [0 : index, 1 : index, 0 : index], kernel = 0 : index, kernel_gemm0 = 2 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
    adf.buffer.location(%9, %c24, %c2, %c0, %c8192) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%8, %c24, %c3, %c0, %c8192) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%7, %c24, %c1, %c16384, %c24576) : (memref<32x32xf32, 2>, index, index, index, index)
    %10 = adf.buffer.create @L1_L1_A_3() : memref<32x32xf32, 2>
    %11 = adf.buffer.create @L1_L1_B_3() : memref<32x32xf32, 2>
    %12 = adf.buffer.create @L1_L1_C_3() : memref<32x32xf32, 2>
    adf.connect(%arg2, %10) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%arg6, %11) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%9, %12) : (memref<32x32xf32, 2>, memref<32x32xf32, 2>)
    %13 = call @kernel_gemm(%10, %11, %12) {adf.kernel, "col, row" = [25 : index, 2 : index], ivs = [1 : index, 1 : index, 0 : index], kernel_gemm = 3 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
    adf.buffer.location(%13, %c25, %c2, %c0, %c8192) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%11, %c25, %c3, %c0, %c8192) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%10, %c25, %c1, %c16384, %c24576) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.connect(%13, %arg7) {write = 1 : index} : (memref<32x32xf32, 2>, !adf.plio<Out, 128>)
    %14 = adf.buffer.create @L1_L1_A_4() : memref<32x32xf32, 2>
    %15 = adf.buffer.create @L1_L1_B_4() : memref<32x32xf32, 2>
    adf.connect(%arg8, %14) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%arg1, %15) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    %16 = call @kernel_gemm0(%14, %15) {adf.kernel, "col, row" = [24 : index, 1 : index], ivs = [0 : index, 0 : index, 1 : index], kernel = 0 : index, kernel_gemm0 = 4 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
    adf.buffer.location(%16, %c25, %c1, %c4096, %c12288) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%15, %c24, %c2, %c16384, %c24576) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%14, %c24, %c0, %c4096, %c12288) : (memref<32x32xf32, 2>, index, index, index, index)
    %17 = adf.buffer.create @L1_L1_A_5() : memref<32x32xf32, 2>
    %18 = adf.buffer.create @L1_L1_B_5() : memref<32x32xf32, 2>
    %19 = adf.buffer.create @L1_L1_C_5() : memref<32x32xf32, 2>
    adf.connect(%arg9, %17) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%arg3, %18) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%16, %19) : (memref<32x32xf32, 2>, memref<32x32xf32, 2>)
    %20 = call @kernel_gemm(%17, %18, %19) {adf.kernel, "col, row" = [25 : index, 1 : index], ivs = [1 : index, 0 : index, 1 : index], kernel_gemm = 5 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
    adf.buffer.location(%20, %c26, %c1, %c0, %c8192) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%18, %c25, %c2, %c16384, %c24576) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%17, %c25, %c0, %c16384, %c24576) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.connect(%20, %arg10) {write = 1 : index} : (memref<32x32xf32, 2>, !adf.plio<Out, 128>)
    %21 = adf.buffer.create @L1_L1_A_6() : memref<32x32xf32, 2>
    %22 = adf.buffer.create @L1_L1_B_6() : memref<32x32xf32, 2>
    adf.connect(%arg8, %21) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%arg5, %22) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    %23 = call @kernel_gemm0(%21, %22) {adf.kernel, "col, row" = [24 : index, 3 : index], ivs = [0 : index, 1 : index, 1 : index], kernel = 0 : index, kernel_gemm0 = 6 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
    adf.buffer.location(%23, %c25, %c3, %c16384, %c24576) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%22, %c24, %c4, %c0, %c8192) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%21, %c24, %c2, %c4096, %c12288) : (memref<32x32xf32, 2>, index, index, index, index)
    %24 = adf.buffer.create @L1_L1_A_7() : memref<32x32xf32, 2>
    %25 = adf.buffer.create @L1_L1_B_7() : memref<32x32xf32, 2>
    %26 = adf.buffer.create @L1_L1_C_7() : memref<32x32xf32, 2>
    adf.connect(%arg9, %24) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%arg6, %25) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%23, %26) : (memref<32x32xf32, 2>, memref<32x32xf32, 2>)
    %27 = call @kernel_gemm(%24, %25, %26) {adf.kernel, "col, row" = [25 : index, 3 : index], ivs = [1 : index, 1 : index, 1 : index], kernel_gemm = 7 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
    adf.buffer.location(%27, %c26, %c3, %c0, %c8192) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%25, %c25, %c4, %c0, %c8192) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%24, %c25, %c2, %c4096, %c12288) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.connect(%27, %arg11) {write = 1 : index} : (memref<32x32xf32, 2>, !adf.plio<Out, 128>)
    return
  }
  func.func @gemm_pl(%arg0: memref<64x16xi128>, %arg1: memref<64x16xi128>, %arg2: memref<64x16xi128>, %arg3: memref<1xi128, "plio">, %arg4: memref<1xi128, "plio">, %arg5: memref<1xi128, "plio">, %arg6: memref<1xi128, "plio">, %arg7: memref<1xi128, "plio">, %arg8: memref<1xi128, "plio">, %arg9: memref<1xi128, "plio">, %arg10: memref<1xi128, "plio">, %arg11: memref<1xi128, "plio">, %arg12: memref<1xi128, "plio">, %arg13: memref<1xi128, "plio">, %arg14: memref<1xi128, "plio">) attributes {adf.pl = true, mem_idx = [0 : i32, 1 : i32, 2 : i32], mem_type = [f32, f32, f32]} {
    %c96 = arith.constant 96 : index
    %c127 = arith.constant 127 : index
    %c64 = arith.constant 64 : index
    %c95 = arith.constant 95 : index
    %c32 = arith.constant 32 : index
    %c63 = arith.constant 63 : index
    %c31 = arith.constant 31 : index
    %c0_i128 = arith.constant 0 : i128
    %c0 = arith.constant 0 : index
    %alloc = memref.alloc() : memref<1xi128, "stream">
    %alloc_0 = memref.alloc() : memref<1xi128, "stream">
    %alloc_1 = memref.alloc() : memref<1xi128, "stream">
    %alloc_2 = memref.alloc() : memref<1xi128, "stream">
    %alloc_3 = memref.alloc() : memref<1xi128, "stream">
    %alloc_4 = memref.alloc() : memref<1xi128, "stream">
    %alloc_5 = memref.alloc() : memref<1xi128, "stream">
    %alloc_6 = memref.alloc() : memref<1xi128, "stream">
    %alloc_7 = memref.alloc() : memref<1xi128, "stream">
    %alloc_8 = memref.alloc() : memref<1xi128, "stream">
    %alloc_9 = memref.alloc() : memref<1xi128, "stream">
    %alloc_10 = memref.alloc() : memref<1xi128, "stream">
    %alloc_11 = memref.alloc() {buffer_type = "uram_t2p", init} : memref<32x8xi128, 1>
    %alloc_12 = memref.alloc() {buffer_type = "uram_t2p", init} : memref<32x8xi128, 1>
    %alloc_13 = memref.alloc() {buffer_type = "uram_t2p", init} : memref<32x8xi128, 1>
    %alloc_14 = memref.alloc() {buffer_type = "uram_t2p", init} : memref<32x8xi128, 1>
    %alloc_15 = memref.alloc() {buffer_type = "bram_s2p"} : memref<32x8xi128, 1>
    %alloc_16 = memref.alloc() {buffer_type = "bram_s2p"} : memref<32x8xi128, 1>
    %alloc_17 = memref.alloc() {buffer_type = "bram_s2p"} : memref<32x8xi128, 1>
    %alloc_18 = memref.alloc() {buffer_type = "bram_s2p"} : memref<32x8xi128, 1>
    %alloc_19 = memref.alloc() {buffer_type = "bram_s2p"} : memref<32x8xi128, 1>
    %alloc_20 = memref.alloc() {buffer_type = "bram_s2p"} : memref<32x8xi128, 1>
    %alloc_21 = memref.alloc() {buffer_type = "bram_s2p"} : memref<32x8xi128, 1>
    %alloc_22 = memref.alloc() {buffer_type = "bram_s2p"} : memref<32x8xi128, 1>
    affine.for %arg15 = 0 to 1 {
      affine.for %arg16 = 0 to 1 {
        affine.for %arg17 = 0 to 1 {
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %alloc_7[0] : memref<1xi128, "stream">
                  affine.store %0, %alloc_19[%arg19 + %arg18 * 32, %arg21 + %arg20 * 8] : memref<32x8xi128, 1>
                } {pipeline_ii = 1 : index}
              }
            }
          } {module = 0 : index}
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 1 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 32 {
                  affine.for %arg22 = 0 to 8 {
                    %0 = affine.load %alloc_19[%arg21 + %arg20 * 32, %arg22 + %arg19 * 8] : memref<32x8xi128, 1>
                    affine.store %0, %arg14[0] : memref<1xi128, "plio">
                  } {pipeline_ii = 1 : index}
                }
              }
            }
          } {module = 1 : index}
        } {Array_Partition, reduction}
      }
    } {send = 3 : index}
    affine.for %arg15 = 0 to 1 {
      affine.for %arg16 = 0 to 1 {
        affine.for %arg17 = 0 to 1 {
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %alloc_5[0] : memref<1xi128, "stream">
                  affine.store %0, %alloc_17[%arg19 + %arg18 * 32, %arg21 + %arg20 * 8] : memref<32x8xi128, 1>
                } {pipeline_ii = 1 : index}
              }
            }
          } {module = 0 : index}
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 1 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 32 {
                  affine.for %arg22 = 0 to 8 {
                    %0 = affine.load %alloc_17[%arg21 + %arg20 * 32, %arg22 + %arg19 * 8] : memref<32x8xi128, 1>
                    affine.store %0, %arg11[0] : memref<1xi128, "plio">
                  } {pipeline_ii = 1 : index}
                }
              }
            }
          } {module = 1 : index}
        } {Array_Partition, reduction}
      }
    } {send = 5 : index}
    affine.for %arg15 = 0 to 1 {
      affine.for %arg16 = 0 to 1 {
        affine.for %arg17 = 0 to 1 {
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %alloc_9[0] : memref<1xi128, "stream">
                  affine.store %0, %alloc_21[%arg19 + %arg18 * 32, %arg21 + %arg20 * 8] : memref<32x8xi128, 1>
                } {pipeline_ii = 1 : index}
              }
            }
          } {module = 0 : index}
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 1 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 32 {
                  affine.for %arg22 = 0 to 8 {
                    %0 = affine.load %alloc_21[%arg21 + %arg20 * 32, %arg22 + %arg19 * 8] : memref<32x8xi128, 1>
                    affine.store %0, %arg10[0] : memref<1xi128, "plio">
                  } {pipeline_ii = 1 : index}
                }
              }
            }
          } {module = 1 : index}
        } {Array_Partition, reduction}
      }
    } {send = 1 : index}
    affine.for %arg15 = 0 to 1 {
      affine.for %arg16 = 0 to 1 {
        affine.for %arg17 = 0 to 1 {
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 1 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 32 {
                  affine.for %arg22 = 0 to 8 {
                    %0 = affine.load %arg8[0] : memref<1xi128, "plio">
                    %1 = affine.load %alloc_12[%arg21 + %arg18 * 32, %arg22 + %arg19 * 8] : memref<32x8xi128, 1>
                    %2 = adf.int_to_apint(%0) : (i128) -> i128
                    %3 = adf.int_to_apint(%1) : (i128) -> i128
                    %4 = adf.int_to_apint(%c0_i128) : (i128) -> i128
                    %5 = adf.get_slice(%2 : i128, %c31, %c0) -> i32
                    %6 = adf.get_slice(%3 : i128, %c31, %c0) -> i32
                    %7 = arith.bitcast %5 : i32 to f32
                    %8 = arith.bitcast %6 : i32 to f32
                    %9 = arith.addf %7, %8 : f32
                    %10 = arith.bitcast %9 : f32 to i32
                    adf.set_slice(%4 : i128, %c31, %c0, %10 : i32)
                    %11 = adf.get_slice(%2 : i128, %c63, %c32) -> i32
                    %12 = adf.get_slice(%3 : i128, %c63, %c32) -> i32
                    %13 = arith.bitcast %11 : i32 to f32
                    %14 = arith.bitcast %12 : i32 to f32
                    %15 = arith.addf %13, %14 : f32
                    %16 = arith.bitcast %15 : f32 to i32
                    adf.set_slice(%4 : i128, %c63, %c32, %16 : i32)
                    %17 = adf.get_slice(%2 : i128, %c95, %c64) -> i32
                    %18 = adf.get_slice(%3 : i128, %c95, %c64) -> i32
                    %19 = arith.bitcast %17 : i32 to f32
                    %20 = arith.bitcast %18 : i32 to f32
                    %21 = arith.addf %19, %20 : f32
                    %22 = arith.bitcast %21 : f32 to i32
                    adf.set_slice(%4 : i128, %c95, %c64, %22 : i32)
                    %23 = adf.get_slice(%2 : i128, %c127, %c96) -> i32
                    %24 = adf.get_slice(%3 : i128, %c127, %c96) -> i32
                    %25 = arith.bitcast %23 : i32 to f32
                    %26 = arith.bitcast %24 : i32 to f32
                    %27 = arith.addf %25, %26 : f32
                    %28 = arith.bitcast %27 : f32 to i32
                    adf.set_slice(%4 : i128, %c127, %c96, %28 : i32)
                    %29 = adf.apint_to_int(%4) : (i128) -> i128
                    affine.store %29, %alloc_12[%arg21 + %arg18 * 32, %arg22 + %arg19 * 8] : memref<32x8xi128, 1>
                  } {pipeline_ii = 1 : index}
                }
              }
            }
          }
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %alloc_12[%arg19 + %arg18 * 32, %arg21 + %arg20 * 8] : memref<32x8xi128, 1>
                  affine.store %0, %alloc_1[0] : memref<1xi128, "stream">
                  affine.store %c0_i128, %alloc_12[%arg19 + %arg18 * 32, %arg21 + %arg20 * 8] : memref<32x8xi128, 1>
                } {pipeline_ii = 1 : index}
              }
            }
          } {hoist}
        } {Array_Partition, reduction}
      }
    } {receive = 2 : index}
    affine.for %arg15 = 0 to 1 {
      affine.for %arg16 = 0 to 1 {
        affine.for %arg17 = 0 to 1 {
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 1 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 32 {
                  affine.for %arg22 = 0 to 8 {
                    %0 = affine.load %arg5[0] : memref<1xi128, "plio">
                    %1 = affine.load %alloc_14[%arg21 + %arg18 * 32, %arg22 + %arg19 * 8] : memref<32x8xi128, 1>
                    %2 = adf.int_to_apint(%0) : (i128) -> i128
                    %3 = adf.int_to_apint(%1) : (i128) -> i128
                    %4 = adf.int_to_apint(%c0_i128) : (i128) -> i128
                    %5 = adf.get_slice(%2 : i128, %c31, %c0) -> i32
                    %6 = adf.get_slice(%3 : i128, %c31, %c0) -> i32
                    %7 = arith.bitcast %5 : i32 to f32
                    %8 = arith.bitcast %6 : i32 to f32
                    %9 = arith.addf %7, %8 : f32
                    %10 = arith.bitcast %9 : f32 to i32
                    adf.set_slice(%4 : i128, %c31, %c0, %10 : i32)
                    %11 = adf.get_slice(%2 : i128, %c63, %c32) -> i32
                    %12 = adf.get_slice(%3 : i128, %c63, %c32) -> i32
                    %13 = arith.bitcast %11 : i32 to f32
                    %14 = arith.bitcast %12 : i32 to f32
                    %15 = arith.addf %13, %14 : f32
                    %16 = arith.bitcast %15 : f32 to i32
                    adf.set_slice(%4 : i128, %c63, %c32, %16 : i32)
                    %17 = adf.get_slice(%2 : i128, %c95, %c64) -> i32
                    %18 = adf.get_slice(%3 : i128, %c95, %c64) -> i32
                    %19 = arith.bitcast %17 : i32 to f32
                    %20 = arith.bitcast %18 : i32 to f32
                    %21 = arith.addf %19, %20 : f32
                    %22 = arith.bitcast %21 : f32 to i32
                    adf.set_slice(%4 : i128, %c95, %c64, %22 : i32)
                    %23 = adf.get_slice(%2 : i128, %c127, %c96) -> i32
                    %24 = adf.get_slice(%3 : i128, %c127, %c96) -> i32
                    %25 = arith.bitcast %23 : i32 to f32
                    %26 = arith.bitcast %24 : i32 to f32
                    %27 = arith.addf %25, %26 : f32
                    %28 = arith.bitcast %27 : f32 to i32
                    adf.set_slice(%4 : i128, %c127, %c96, %28 : i32)
                    %29 = adf.apint_to_int(%4) : (i128) -> i128
                    affine.store %29, %alloc_14[%arg21 + %arg18 * 32, %arg22 + %arg19 * 8] : memref<32x8xi128, 1>
                  } {pipeline_ii = 1 : index}
                }
              }
            }
          }
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %alloc_14[%arg19 + %arg18 * 32, %arg21 + %arg20 * 8] : memref<32x8xi128, 1>
                  affine.store %0, %alloc[0] : memref<1xi128, "stream">
                  affine.store %c0_i128, %alloc_14[%arg19 + %arg18 * 32, %arg21 + %arg20 * 8] : memref<32x8xi128, 1>
                } {pipeline_ii = 1 : index}
              }
            }
          } {hoist}
        } {Array_Partition, reduction}
      }
    } {receive = 0 : index}
    affine.for %arg15 = 0 to 1 {
      affine.for %arg16 = 0 to 1 {
        affine.for %arg17 = 0 to 1 {
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 1 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 32 {
                  affine.for %arg22 = 0 to 8 {
                    %0 = affine.load %arg3[0] : memref<1xi128, "plio">
                    %1 = affine.load %alloc_11[%arg21 + %arg18 * 32, %arg22 + %arg19 * 8] : memref<32x8xi128, 1>
                    %2 = adf.int_to_apint(%0) : (i128) -> i128
                    %3 = adf.int_to_apint(%1) : (i128) -> i128
                    %4 = adf.int_to_apint(%c0_i128) : (i128) -> i128
                    %5 = adf.get_slice(%2 : i128, %c31, %c0) -> i32
                    %6 = adf.get_slice(%3 : i128, %c31, %c0) -> i32
                    %7 = arith.bitcast %5 : i32 to f32
                    %8 = arith.bitcast %6 : i32 to f32
                    %9 = arith.addf %7, %8 : f32
                    %10 = arith.bitcast %9 : f32 to i32
                    adf.set_slice(%4 : i128, %c31, %c0, %10 : i32)
                    %11 = adf.get_slice(%2 : i128, %c63, %c32) -> i32
                    %12 = adf.get_slice(%3 : i128, %c63, %c32) -> i32
                    %13 = arith.bitcast %11 : i32 to f32
                    %14 = arith.bitcast %12 : i32 to f32
                    %15 = arith.addf %13, %14 : f32
                    %16 = arith.bitcast %15 : f32 to i32
                    adf.set_slice(%4 : i128, %c63, %c32, %16 : i32)
                    %17 = adf.get_slice(%2 : i128, %c95, %c64) -> i32
                    %18 = adf.get_slice(%3 : i128, %c95, %c64) -> i32
                    %19 = arith.bitcast %17 : i32 to f32
                    %20 = arith.bitcast %18 : i32 to f32
                    %21 = arith.addf %19, %20 : f32
                    %22 = arith.bitcast %21 : f32 to i32
                    adf.set_slice(%4 : i128, %c95, %c64, %22 : i32)
                    %23 = adf.get_slice(%2 : i128, %c127, %c96) -> i32
                    %24 = adf.get_slice(%3 : i128, %c127, %c96) -> i32
                    %25 = arith.bitcast %23 : i32 to f32
                    %26 = arith.bitcast %24 : i32 to f32
                    %27 = arith.addf %25, %26 : f32
                    %28 = arith.bitcast %27 : f32 to i32
                    adf.set_slice(%4 : i128, %c127, %c96, %28 : i32)
                    %29 = adf.apint_to_int(%4) : (i128) -> i128
                    affine.store %29, %alloc_11[%arg21 + %arg18 * 32, %arg22 + %arg19 * 8] : memref<32x8xi128, 1>
                  } {pipeline_ii = 1 : index}
                }
              }
            }
          }
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %alloc_11[%arg19 + %arg18 * 32, %arg21 + %arg20 * 8] : memref<32x8xi128, 1>
                  affine.store %0, %alloc_2[0] : memref<1xi128, "stream">
                  affine.store %c0_i128, %alloc_11[%arg19 + %arg18 * 32, %arg21 + %arg20 * 8] : memref<32x8xi128, 1>
                } {pipeline_ii = 1 : index}
              }
            }
          } {hoist}
        } {Array_Partition, reduction}
      }
    } {receive = 3 : index}
    affine.for %arg15 = 0 to 1 {
      affine.for %arg16 = 0 to 1 {
        affine.for %arg17 = 0 to 1 {
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %alloc_4[0] : memref<1xi128, "stream">
                  affine.store %0, %alloc_16[%arg19 + %arg18 * 32, %arg21 + %arg20 * 8] : memref<32x8xi128, 1>
                } {pipeline_ii = 1 : index}
              }
            }
          } {module = 0 : index}
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 1 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 32 {
                  affine.for %arg22 = 0 to 8 {
                    %0 = affine.load %alloc_16[%arg21 + %arg18 * 32, %arg22 + %arg20 * 8] : memref<32x8xi128, 1>
                    affine.store %0, %arg9[0] : memref<1xi128, "plio">
                  } {pipeline_ii = 1 : index}
                }
              }
            }
          } {module = 1 : index}
        } {Array_Partition, reduction}
      }
    } {send = 6 : index}
    affine.for %arg15 = 0 to 1 {
      affine.for %arg16 = 0 to 1 {
        affine.for %arg17 = 0 to 1 {
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %alloc_10[0] : memref<1xi128, "stream">
                  affine.store %0, %alloc_22[%arg19 + %arg18 * 32, %arg21 + %arg20 * 8] : memref<32x8xi128, 1>
                } {pipeline_ii = 1 : index}
              }
            }
          } {module = 0 : index}
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 1 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 32 {
                  affine.for %arg22 = 0 to 8 {
                    %0 = affine.load %alloc_22[%arg21 + %arg18 * 32, %arg22 + %arg20 * 8] : memref<32x8xi128, 1>
                    affine.store %0, %arg12[0] : memref<1xi128, "plio">
                  } {pipeline_ii = 1 : index}
                }
              }
            }
          } {module = 1 : index}
        } {Array_Partition, reduction}
      }
    } {send = 0 : index}
    affine.for %arg15 = 0 to 1 {
      affine.for %arg16 = 0 to 1 {
        affine.for %arg17 = 0 to 1 {
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %alloc_6[0] : memref<1xi128, "stream">
                  affine.store %0, %alloc_18[%arg19 + %arg18 * 32, %arg21 + %arg20 * 8] : memref<32x8xi128, 1>
                } {pipeline_ii = 1 : index}
              }
            }
          } {module = 0 : index}
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 1 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 32 {
                  affine.for %arg22 = 0 to 8 {
                    %0 = affine.load %alloc_18[%arg21 + %arg20 * 32, %arg22 + %arg19 * 8] : memref<32x8xi128, 1>
                    affine.store %0, %arg7[0] : memref<1xi128, "plio">
                  } {pipeline_ii = 1 : index}
                }
              }
            }
          } {module = 1 : index}
        } {Array_Partition, reduction}
      }
    } {send = 4 : index}
    affine.for %arg15 = 0 to 1 {
      affine.for %arg16 = 0 to 1 {
        affine.for %arg17 = 0 to 1 {
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %alloc_2[0] : memref<1xi128, "stream">
                  affine.store %0, %arg2[%arg19 + %arg18 * 352 + %arg15 * 1408 + 32, %arg21 + %arg20 * 64 + %arg16 * 384 + 8] : memref<64x16xi128>
                } {pipeline_ii = 1 : index}
              }
            }
          } {hoist, merge}
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %alloc_1[0] : memref<1xi128, "stream">
                  affine.store %0, %arg2[%arg19 + %arg18 * 352 + %arg15 * 1408 + 32, %arg21 + %arg20 * 64 + %arg16 * 384] : memref<64x16xi128>
                } {pipeline_ii = 1 : index}
              }
            }
          } {hoist, merge}
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %alloc_0[0] : memref<1xi128, "stream">
                  affine.store %0, %arg2[%arg19 + %arg18 * 352 + %arg15 * 1408, %arg21 + %arg20 * 64 + %arg16 * 384 + 8] : memref<64x16xi128>
                } {pipeline_ii = 1 : index}
              }
            }
          } {hoist, merge}
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %alloc[0] : memref<1xi128, "stream">
                  affine.store %0, %arg2[%arg19 + %arg18 * 352 + %arg15 * 1408, %arg21 + %arg20 * 64 + %arg16 * 384] : memref<64x16xi128>
                } {pipeline_ii = 1 : index}
              }
            }
          } {hoist, merge}
        } {Array_Partition, reduction}
      }
    } {store = 0 : index}
    affine.for %arg15 = 0 to 1 {
      affine.for %arg16 = 0 to 1 {
        affine.for %arg17 = 0 to 1 {
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 1 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 32 {
                  affine.for %arg22 = 0 to 8 {
                    %0 = affine.load %arg13[0] : memref<1xi128, "plio">
                    %1 = affine.load %alloc_13[%arg21 + %arg18 * 32, %arg22 + %arg19 * 8] : memref<32x8xi128, 1>
                    %2 = adf.int_to_apint(%0) : (i128) -> i128
                    %3 = adf.int_to_apint(%1) : (i128) -> i128
                    %4 = adf.int_to_apint(%c0_i128) : (i128) -> i128
                    %5 = adf.get_slice(%2 : i128, %c31, %c0) -> i32
                    %6 = adf.get_slice(%3 : i128, %c31, %c0) -> i32
                    %7 = arith.bitcast %5 : i32 to f32
                    %8 = arith.bitcast %6 : i32 to f32
                    %9 = arith.addf %7, %8 : f32
                    %10 = arith.bitcast %9 : f32 to i32
                    adf.set_slice(%4 : i128, %c31, %c0, %10 : i32)
                    %11 = adf.get_slice(%2 : i128, %c63, %c32) -> i32
                    %12 = adf.get_slice(%3 : i128, %c63, %c32) -> i32
                    %13 = arith.bitcast %11 : i32 to f32
                    %14 = arith.bitcast %12 : i32 to f32
                    %15 = arith.addf %13, %14 : f32
                    %16 = arith.bitcast %15 : f32 to i32
                    adf.set_slice(%4 : i128, %c63, %c32, %16 : i32)
                    %17 = adf.get_slice(%2 : i128, %c95, %c64) -> i32
                    %18 = adf.get_slice(%3 : i128, %c95, %c64) -> i32
                    %19 = arith.bitcast %17 : i32 to f32
                    %20 = arith.bitcast %18 : i32 to f32
                    %21 = arith.addf %19, %20 : f32
                    %22 = arith.bitcast %21 : f32 to i32
                    adf.set_slice(%4 : i128, %c95, %c64, %22 : i32)
                    %23 = adf.get_slice(%2 : i128, %c127, %c96) -> i32
                    %24 = adf.get_slice(%3 : i128, %c127, %c96) -> i32
                    %25 = arith.bitcast %23 : i32 to f32
                    %26 = arith.bitcast %24 : i32 to f32
                    %27 = arith.addf %25, %26 : f32
                    %28 = arith.bitcast %27 : f32 to i32
                    adf.set_slice(%4 : i128, %c127, %c96, %28 : i32)
                    %29 = adf.apint_to_int(%4) : (i128) -> i128
                    affine.store %29, %alloc_13[%arg21 + %arg18 * 32, %arg22 + %arg19 * 8] : memref<32x8xi128, 1>
                  } {pipeline_ii = 1 : index}
                }
              }
            }
          }
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %alloc_13[%arg19 + %arg18 * 32, %arg21 + %arg20 * 8] : memref<32x8xi128, 1>
                  affine.store %0, %alloc_0[0] : memref<1xi128, "stream">
                  affine.store %c0_i128, %alloc_13[%arg19 + %arg18 * 32, %arg21 + %arg20 * 8] : memref<32x8xi128, 1>
                } {pipeline_ii = 1 : index}
              }
            }
          } {hoist}
        } {Array_Partition, reduction}
      }
    } {receive = 1 : index}
    affine.for %arg15 = 0 to 1 {
      affine.for %arg16 = 0 to 1 {
        affine.for %arg17 = 0 to 1 {
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %arg0[%arg19 + %arg18 * 352 + %arg15 * 1408, %arg21 + %arg20 * 32 + %arg17 * 32] : memref<64x16xi128>
                  affine.store %0, %alloc_10[0] : memref<1xi128, "stream">
                } {pipeline_ii = 1 : index}
              }
            }
          } {merge}
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %arg0[%arg19 + %arg18 * 352 + %arg15 * 1408, %arg21 + %arg20 * 32 + %arg17 * 32 + 8] : memref<64x16xi128>
                  affine.store %0, %alloc_8[0] : memref<1xi128, "stream">
                } {pipeline_ii = 1 : index}
              }
            }
          } {merge}
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %arg0[%arg19 + %arg18 * 352 + %arg15 * 1408 + 32, %arg21 + %arg20 * 32 + %arg17 * 32] : memref<64x16xi128>
                  affine.store %0, %alloc_4[0] : memref<1xi128, "stream">
                } {pipeline_ii = 1 : index}
              }
            }
          } {merge}
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %arg0[%arg19 + %arg18 * 352 + %arg15 * 1408 + 32, %arg21 + %arg20 * 32 + %arg17 * 32 + 8] : memref<64x16xi128>
                  affine.store %0, %alloc_3[0] : memref<1xi128, "stream">
                } {pipeline_ii = 1 : index}
              }
            }
          } {merge}
        } {Array_Partition, reduction}
      }
    } {load = 0 : index}
    affine.for %arg15 = 0 to 1 {
      affine.for %arg16 = 0 to 1 {
        affine.for %arg17 = 0 to 1 {
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %arg1[%arg19 + %arg18 * 128 + %arg17 * 128, %arg21 + %arg20 * 64 + %arg16 * 384] : memref<64x16xi128>
                  affine.store %0, %alloc_9[0] : memref<1xi128, "stream">
                } {pipeline_ii = 1 : index}
              }
            }
          } {merge}
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %arg1[%arg19 + %arg18 * 128 + %arg17 * 128 + 32, %arg21 + %arg20 * 64 + %arg16 * 384] : memref<64x16xi128>
                  affine.store %0, %alloc_7[0] : memref<1xi128, "stream">
                } {pipeline_ii = 1 : index}
              }
            }
          } {merge}
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %arg1[%arg19 + %arg18 * 128 + %arg17 * 128, %arg21 + %arg20 * 64 + %arg16 * 384 + 8] : memref<64x16xi128>
                  affine.store %0, %alloc_6[0] : memref<1xi128, "stream">
                } {pipeline_ii = 1 : index}
              }
            }
          } {merge}
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %arg1[%arg19 + %arg18 * 128 + %arg17 * 128 + 32, %arg21 + %arg20 * 64 + %arg16 * 384 + 8] : memref<64x16xi128>
                  affine.store %0, %alloc_5[0] : memref<1xi128, "stream">
                } {pipeline_ii = 1 : index}
              }
            }
          } {merge}
        } {Array_Partition, reduction}
      }
    } {load = 1 : index}
    affine.for %arg15 = 0 to 1 {
      affine.for %arg16 = 0 to 1 {
        affine.for %arg17 = 0 to 1 {
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %alloc_3[0] : memref<1xi128, "stream">
                  affine.store %0, %alloc_15[%arg19 + %arg18 * 32, %arg21 + %arg20 * 8] : memref<32x8xi128, 1>
                } {pipeline_ii = 1 : index}
              }
            }
          } {module = 0 : index}
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 1 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 32 {
                  affine.for %arg22 = 0 to 8 {
                    %0 = affine.load %alloc_15[%arg21 + %arg18 * 32, %arg22 + %arg20 * 8] : memref<32x8xi128, 1>
                    affine.store %0, %arg6[0] : memref<1xi128, "plio">
                  } {pipeline_ii = 1 : index}
                }
              }
            }
          } {module = 1 : index}
        } {Array_Partition, reduction}
      }
    } {send = 7 : index}
    affine.for %arg15 = 0 to 1 {
      affine.for %arg16 = 0 to 1 {
        affine.for %arg17 = 0 to 1 {
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 32 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 8 {
                  %0 = affine.load %alloc_8[0] : memref<1xi128, "stream">
                  affine.store %0, %alloc_20[%arg19 + %arg18 * 32, %arg21 + %arg20 * 8] : memref<32x8xi128, 1>
                } {pipeline_ii = 1 : index}
              }
            }
          } {module = 0 : index}
          affine.for %arg18 = 0 to 1 {
            affine.for %arg19 = 0 to 1 {
              affine.for %arg20 = 0 to 1 {
                affine.for %arg21 = 0 to 32 {
                  affine.for %arg22 = 0 to 8 {
                    %0 = affine.load %alloc_20[%arg21 + %arg18 * 32, %arg22 + %arg20 * 8] : memref<32x8xi128, 1>
                    affine.store %0, %arg4[0] : memref<1xi128, "plio">
                  } {pipeline_ii = 1 : index}
                }
              }
            }
          } {module = 1 : index}
        } {Array_Partition, reduction}
      }
    } {send = 2 : index}
    return
  }
  func.func @gemm(%arg0: memref<64x16xi128>, %arg1: memref<64x16xi128>, %arg2: memref<64x16xi128>, %arg3: memref<1xi128, "plio">, %arg4: memref<1xi128, "plio">, %arg5: memref<1xi128, "plio">, %arg6: memref<1xi128, "plio">, %arg7: memref<1xi128, "plio">, %arg8: memref<1xi128, "plio">, %arg9: memref<1xi128, "plio">, %arg10: memref<1xi128, "plio">, %arg11: memref<1xi128, "plio">, %arg12: memref<1xi128, "plio">, %arg13: memref<1xi128, "plio">, %arg14: memref<1xi128, "plio">) attributes {adf.func, plio = true} {
    %0 = adf.graph.io(PLIO) : !adf.plio<Out, 128>
    adf.connect(%0, %arg3) {top_config} : (!adf.plio<Out, 128>, memref<1xi128, "plio">)
    %1 = adf.graph.io(PLIO) : !adf.plio<Out, 128>
    adf.connect(%1, %arg8) {top_config} : (!adf.plio<Out, 128>, memref<1xi128, "plio">)
    %2 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    adf.connect(%arg6, %2) {top_config} : (memref<1xi128, "plio">, !adf.plio<In, 128>)
    %3 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    adf.connect(%arg9, %3) {top_config} : (memref<1xi128, "plio">, !adf.plio<In, 128>)
    %4 = adf.graph.io(PLIO) : !adf.plio<Out, 128>
    adf.connect(%4, %arg13) {top_config} : (!adf.plio<Out, 128>, memref<1xi128, "plio">)
    %5 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    adf.connect(%arg11, %5) {top_config} : (memref<1xi128, "plio">, !adf.plio<In, 128>)
    %6 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    adf.connect(%arg7, %6) {top_config} : (memref<1xi128, "plio">, !adf.plio<In, 128>)
    %7 = adf.graph.io(PLIO) : !adf.plio<Out, 128>
    adf.connect(%7, %arg5) {top_config} : (!adf.plio<Out, 128>, memref<1xi128, "plio">)
    %8 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    adf.connect(%arg14, %8) {top_config} : (memref<1xi128, "plio">, !adf.plio<In, 128>)
    %9 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    adf.connect(%arg4, %9) {top_config} : (memref<1xi128, "plio">, !adf.plio<In, 128>)
    %10 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    adf.connect(%arg10, %10) {top_config} : (memref<1xi128, "plio">, !adf.plio<In, 128>)
    %11 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    adf.connect(%arg12, %11) {top_config} : (memref<1xi128, "plio">, !adf.plio<In, 128>)
    adf.cell.launch @adf_cell0 {
      func.call @adf_cell0(%11, %10, %9, %8, %7, %6, %5, %4, %3, %2, %1, %0) {adf.cell} : (!adf.plio<In, 128>, !adf.plio<In, 128>, !adf.plio<In, 128>, !adf.plio<In, 128>, !adf.plio<Out, 128>, !adf.plio<In, 128>, !adf.plio<In, 128>, !adf.plio<Out, 128>, !adf.plio<In, 128>, !adf.plio<In, 128>, !adf.plio<Out, 128>, !adf.plio<Out, 128>) -> ()
      adf.cell.launch.end
    }
    adf.pl.launch @gemm_pl {
      func.call @gemm_pl(%arg0, %arg1, %arg2, %arg3, %arg4, %arg5, %arg6, %arg7, %arg8, %arg9, %arg10, %arg11, %arg12, %arg13, %arg14) {adf.pl} : (memref<64x16xi128>, memref<64x16xi128>, memref<64x16xi128>, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">) -> ()
      adf.pl.launch.wait
    }
    return
  }
  func.func @top(%arg0: memref<64x16xi128>, %arg1: memref<64x16xi128>, %arg2: memref<64x16xi128>, %arg3: memref<1xi128, "plio">, %arg4: memref<1xi128, "plio">, %arg5: memref<1xi128, "plio">, %arg6: memref<1xi128, "plio">, %arg7: memref<1xi128, "plio">, %arg8: memref<1xi128, "plio">, %arg9: memref<1xi128, "plio">, %arg10: memref<1xi128, "plio">, %arg11: memref<1xi128, "plio">, %arg12: memref<1xi128, "plio">, %arg13: memref<1xi128, "plio">, %arg14: memref<1xi128, "plio">) attributes {outArgs = [2 : i32], top_func = "plio"} {
    call @gemm_pl(%arg0, %arg1, %arg2, %arg3, %arg4, %arg5, %arg6, %arg7, %arg8, %arg9, %arg10, %arg11, %arg12, %arg13, %arg14) : (memref<64x16xi128>, memref<64x16xi128>, memref<64x16xi128>, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">) -> ()
    return
  }
  func.func private @gemm_host(memref<64x64xf32>, memref<64x64xf32>, memref<64x64xf32>) attributes {origin_func = "gemm"}
  func.func @top_host(%arg0: memref<64x64xf32>, %arg1: memref<64x64xf32>, %arg2: memref<64x64xf32>) attributes {origin_func = "top", outArgs = [2 : i32], top_host} {
    call @gemm_host(%arg0, %arg1, %arg2) {origin_func = "gemm"} : (memref<64x64xf32>, memref<64x64xf32>, memref<64x64xf32>) -> ()
    return
  }
}
