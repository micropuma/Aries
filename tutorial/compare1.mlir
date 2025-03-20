// -----// IR Dump After AriesIOPlacement (aries-io-placement) //----- //
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
    %c24 = arith.constant 24 : index
    %c1 = arith.constant 1 : index
    %c0 = arith.constant 0 : index
    %c8192 = arith.constant 8192 : index
    %c23 = arith.constant 23 : index
    %c0_0 = arith.constant 0 : index
    %c0_1 = arith.constant 0 : index
    %c8192_2 = arith.constant 8192 : index
    %c24_3 = arith.constant 24 : index
    %c0_4 = arith.constant 0 : index
    %c0_5 = arith.constant 0 : index
    %c8192_6 = arith.constant 8192 : index
    %2 = call @kernel_gemm0(%0, %1) {adf.kernel, "col, row" = [24 : index, 0 : index], ivs = [0 : index, 0 : index, 0 : index], kernel = 0 : index, kernel_gemm0 = 0 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
    adf.buffer.location(%2, %c24_3, %c0_4, %c0_5, %c8192_6) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%1, %c23, %c0_0, %c0_1, %c8192_2) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%0, %c24, %c1, %c0, %c8192) : (memref<32x32xf32, 2>, index, index, index, index)
    %3 = adf.buffer.create @L1_L1_A_1() : memref<32x32xf32, 2>
    %4 = adf.buffer.create @L1_L1_B_1() : memref<32x32xf32, 2>
    %5 = adf.buffer.create @L1_L1_C_1() : memref<32x32xf32, 2>
    adf.connect(%arg2, %3) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%arg3, %4) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%2, %5) : (memref<32x32xf32, 2>, memref<32x32xf32, 2>)
    %c25 = arith.constant 25 : index
    %c1_7 = arith.constant 1 : index
    %c0_8 = arith.constant 0 : index
    %c8192_9 = arith.constant 8192 : index
    %c24_10 = arith.constant 24 : index
    %c0_11 = arith.constant 0 : index
    %c16384 = arith.constant 16384 : index
    %c24576 = arith.constant 24576 : index
    %c25_12 = arith.constant 25 : index
    %c0_13 = arith.constant 0 : index
    %c0_14 = arith.constant 0 : index
    %c8192_15 = arith.constant 8192 : index
    %6 = call @kernel_gemm(%3, %4, %5) {adf.kernel, "col, row" = [25 : index, 0 : index], ivs = [1 : index, 0 : index, 0 : index], kernel_gemm = 1 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
    adf.buffer.location(%6, %c25_12, %c0_13, %c0_14, %c8192_15) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%4, %c24_10, %c0_11, %c16384, %c24576) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%3, %c25, %c1_7, %c0_8, %c8192_9) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.connect(%6, %arg4) {write = 1 : index} : (memref<32x32xf32, 2>, !adf.plio<Out, 128>)
    %7 = adf.buffer.create @L1_L1_A_2() : memref<32x32xf32, 2>
    %8 = adf.buffer.create @L1_L1_B_2() : memref<32x32xf32, 2>
    adf.connect(%arg0, %7) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%arg5, %8) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    %c24_16 = arith.constant 24 : index
    %c1_17 = arith.constant 1 : index
    %c16384_18 = arith.constant 16384 : index
    %c24576_19 = arith.constant 24576 : index
    %c24_20 = arith.constant 24 : index
    %c3 = arith.constant 3 : index
    %c0_21 = arith.constant 0 : index
    %c8192_22 = arith.constant 8192 : index
    %c24_23 = arith.constant 24 : index
    %c2 = arith.constant 2 : index
    %c0_24 = arith.constant 0 : index
    %c8192_25 = arith.constant 8192 : index
    %9 = call @kernel_gemm0(%7, %8) {adf.kernel, "col, row" = [24 : index, 2 : index], ivs = [0 : index, 1 : index, 0 : index], kernel = 0 : index, kernel_gemm0 = 2 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
    adf.buffer.location(%9, %c24_23, %c2, %c0_24, %c8192_25) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%8, %c24_20, %c3, %c0_21, %c8192_22) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%7, %c24_16, %c1_17, %c16384_18, %c24576_19) : (memref<32x32xf32, 2>, index, index, index, index)
    %10 = adf.buffer.create @L1_L1_A_3() : memref<32x32xf32, 2>
    %11 = adf.buffer.create @L1_L1_B_3() : memref<32x32xf32, 2>
    %12 = adf.buffer.create @L1_L1_C_3() : memref<32x32xf32, 2>
    adf.connect(%arg2, %10) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%arg6, %11) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%9, %12) : (memref<32x32xf32, 2>, memref<32x32xf32, 2>)
    %c25_26 = arith.constant 25 : index
    %c1_27 = arith.constant 1 : index
    %c16384_28 = arith.constant 16384 : index
    %c24576_29 = arith.constant 24576 : index
    %c25_30 = arith.constant 25 : index
    %c3_31 = arith.constant 3 : index
    %c0_32 = arith.constant 0 : index
    %c8192_33 = arith.constant 8192 : index
    %c25_34 = arith.constant 25 : index
    %c2_35 = arith.constant 2 : index
    %c0_36 = arith.constant 0 : index
    %c8192_37 = arith.constant 8192 : index
    %13 = call @kernel_gemm(%10, %11, %12) {adf.kernel, "col, row" = [25 : index, 2 : index], ivs = [1 : index, 1 : index, 0 : index], kernel_gemm = 3 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
    adf.buffer.location(%13, %c25_34, %c2_35, %c0_36, %c8192_37) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%11, %c25_30, %c3_31, %c0_32, %c8192_33) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%10, %c25_26, %c1_27, %c16384_28, %c24576_29) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.connect(%13, %arg7) {write = 1 : index} : (memref<32x32xf32, 2>, !adf.plio<Out, 128>)
    %14 = adf.buffer.create @L1_L1_A_4() : memref<32x32xf32, 2>
    %15 = adf.buffer.create @L1_L1_B_4() : memref<32x32xf32, 2>
    adf.connect(%arg8, %14) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%arg1, %15) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    %c24_38 = arith.constant 24 : index
    %c0_39 = arith.constant 0 : index
    %c4096 = arith.constant 4096 : index
    %c12288 = arith.constant 12288 : index
    %c24_40 = arith.constant 24 : index
    %c2_41 = arith.constant 2 : index
    %c16384_42 = arith.constant 16384 : index
    %c24576_43 = arith.constant 24576 : index
    %c25_44 = arith.constant 25 : index
    %c1_45 = arith.constant 1 : index
    %c4096_46 = arith.constant 4096 : index
    %c12288_47 = arith.constant 12288 : index
    %16 = call @kernel_gemm0(%14, %15) {adf.kernel, "col, row" = [24 : index, 1 : index], ivs = [0 : index, 0 : index, 1 : index], kernel = 0 : index, kernel_gemm0 = 4 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
    adf.buffer.location(%16, %c25_44, %c1_45, %c4096_46, %c12288_47) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%15, %c24_40, %c2_41, %c16384_42, %c24576_43) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%14, %c24_38, %c0_39, %c4096, %c12288) : (memref<32x32xf32, 2>, index, index, index, index)
    %17 = adf.buffer.create @L1_L1_A_5() : memref<32x32xf32, 2>
    %18 = adf.buffer.create @L1_L1_B_5() : memref<32x32xf32, 2>
    %19 = adf.buffer.create @L1_L1_C_5() : memref<32x32xf32, 2>
    adf.connect(%arg9, %17) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%arg3, %18) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%16, %19) : (memref<32x32xf32, 2>, memref<32x32xf32, 2>)
    %c25_48 = arith.constant 25 : index
    %c0_49 = arith.constant 0 : index
    %c16384_50 = arith.constant 16384 : index
    %c24576_51 = arith.constant 24576 : index
    %c25_52 = arith.constant 25 : index
    %c2_53 = arith.constant 2 : index
    %c16384_54 = arith.constant 16384 : index
    %c24576_55 = arith.constant 24576 : index
    %c26 = arith.constant 26 : index
    %c1_56 = arith.constant 1 : index
    %c0_57 = arith.constant 0 : index
    %c8192_58 = arith.constant 8192 : index
    %20 = call @kernel_gemm(%17, %18, %19) {adf.kernel, "col, row" = [25 : index, 1 : index], ivs = [1 : index, 0 : index, 1 : index], kernel_gemm = 5 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
    adf.buffer.location(%20, %c26, %c1_56, %c0_57, %c8192_58) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%18, %c25_52, %c2_53, %c16384_54, %c24576_55) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%17, %c25_48, %c0_49, %c16384_50, %c24576_51) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.connect(%20, %arg10) {write = 1 : index} : (memref<32x32xf32, 2>, !adf.plio<Out, 128>)
    %21 = adf.buffer.create @L1_L1_A_6() : memref<32x32xf32, 2>
    %22 = adf.buffer.create @L1_L1_B_6() : memref<32x32xf32, 2>
    adf.connect(%arg8, %21) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%arg5, %22) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    %c24_59 = arith.constant 24 : index
    %c2_60 = arith.constant 2 : index
    %c4096_61 = arith.constant 4096 : index
    %c12288_62 = arith.constant 12288 : index
    %c24_63 = arith.constant 24 : index
    %c4 = arith.constant 4 : index
    %c0_64 = arith.constant 0 : index
    %c8192_65 = arith.constant 8192 : index
    %c25_66 = arith.constant 25 : index
    %c3_67 = arith.constant 3 : index
    %c16384_68 = arith.constant 16384 : index
    %c24576_69 = arith.constant 24576 : index
    %23 = call @kernel_gemm0(%21, %22) {adf.kernel, "col, row" = [24 : index, 3 : index], ivs = [0 : index, 1 : index, 1 : index], kernel = 0 : index, kernel_gemm0 = 6 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
    adf.buffer.location(%23, %c25_66, %c3_67, %c16384_68, %c24576_69) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%22, %c24_63, %c4, %c0_64, %c8192_65) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%21, %c24_59, %c2_60, %c4096_61, %c12288_62) : (memref<32x32xf32, 2>, index, index, index, index)
    %24 = adf.buffer.create @L1_L1_A_7() : memref<32x32xf32, 2>
    %25 = adf.buffer.create @L1_L1_B_7() : memref<32x32xf32, 2>
    %26 = adf.buffer.create @L1_L1_C_7() : memref<32x32xf32, 2>
    adf.connect(%arg9, %24) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%arg6, %25) : (!adf.plio<In, 128>, memref<32x32xf32, 2>)
    adf.connect(%23, %26) : (memref<32x32xf32, 2>, memref<32x32xf32, 2>)
    %c25_70 = arith.constant 25 : index
    %c2_71 = arith.constant 2 : index
    %c4096_72 = arith.constant 4096 : index
    %c12288_73 = arith.constant 12288 : index
    %c25_74 = arith.constant 25 : index
    %c4_75 = arith.constant 4 : index
    %c0_76 = arith.constant 0 : index
    %c8192_77 = arith.constant 8192 : index
    %c26_78 = arith.constant 26 : index
    %c3_79 = arith.constant 3 : index
    %c0_80 = arith.constant 0 : index
    %c8192_81 = arith.constant 8192 : index
    %27 = call @kernel_gemm(%24, %25, %26) {adf.kernel, "col, row" = [25 : index, 3 : index], ivs = [1 : index, 1 : index, 1 : index], kernel_gemm = 7 : index} : (memref<32x32xf32, 2>, memref<32x32xf32, 2>, memref<32x32xf32, 2>) -> memref<32x32xf32, 2>
    adf.buffer.location(%27, %c26_78, %c3_79, %c0_80, %c8192_81) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%25, %c25_74, %c4_75, %c0_76, %c8192_77) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.buffer.location(%24, %c25_70, %c2_71, %c4096_72, %c12288_73) : (memref<32x32xf32, 2>, index, index, index, index)
    adf.connect(%27, %arg11) {write = 1 : index} : (memref<32x32xf32, 2>, !adf.plio<Out, 128>)
    return
  }
  func.func @gemm(%arg0: memref<64x16xi128>, %arg1: memref<64x16xi128>, %arg2: memref<64x16xi128>) attributes {adf.func, plio = true} {
    %0 = adf.graph.io(PLIO) : !adf.plio<Out, 128>
    %1 = adf.graph.io(PLIO) : !adf.plio<Out, 128>
    %2 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    %3 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    %4 = adf.graph.io(PLIO) : !adf.plio<Out, 128>
    %5 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    %6 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    %7 = adf.graph.io(PLIO) : !adf.plio<Out, 128>
    %8 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    %9 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    %10 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    %11 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    adf.cell.launch @adf_cell0 {
      %c8 = arith.constant 8 : index
      %c1 = arith.constant 1 : index
      %c32 = arith.constant 32 : index
      affine.for %arg3 = 0 to 1 {
        affine.for %arg4 = 0 to 1 {
          affine.for %arg5 = 0 to 1 {
            affine.for %arg6 = 0 to 1 {
              affine.for %arg7 = 0 to 1 {
                affine.for %arg8 = 0 to 1 {
                  %12 = affine.apply #map(%arg6, %arg3)
                  %13 = affine.apply #map1(%arg8, %arg5)
                  %14 = affine.apply #map2(%arg8, %arg5)
                  adf.io.push(%arg0[%12, %13] [%c32, %c8] [%c1, %c1], %11) {type = f32} : (memref<64x16xi128>, !adf.plio<In, 128>)
                  %15 = affine.apply #map3(%arg7, %arg4)
                  adf.io.push(%arg1[%14, %15] [%c32, %c8] [%c1, %c1], %10) {type = f32} : (memref<64x16xi128>, !adf.plio<In, 128>)
                  %16 = affine.apply #map4(%arg8, %arg5)
                  %17 = affine.apply #map5(%arg8, %arg5)
                  adf.io.push(%arg0[%12, %16] [%c32, %c8] [%c1, %c1], %9) {type = f32} : (memref<64x16xi128>, !adf.plio<In, 128>)
                  adf.io.push(%arg1[%17, %15] [%c32, %c8] [%c1, %c1], %8) {type = f32} : (memref<64x16xi128>, !adf.plio<In, 128>)
                  %18 = affine.apply #map6(%arg7, %arg4)
                  adf.io.push(%arg1[%14, %18] [%c32, %c8] [%c1, %c1], %6) {type = f32} : (memref<64x16xi128>, !adf.plio<In, 128>)
                  adf.io.push(%arg1[%17, %18] [%c32, %c8] [%c1, %c1], %5) {type = f32} : (memref<64x16xi128>, !adf.plio<In, 128>)
                  %19 = affine.apply #map7(%arg6, %arg3)
                  adf.io.push(%arg0[%19, %13] [%c32, %c8] [%c1, %c1], %3) {type = f32} : (memref<64x16xi128>, !adf.plio<In, 128>)
                  adf.io.push(%arg0[%19, %16] [%c32, %c8] [%c1, %c1], %2) {type = f32} : (memref<64x16xi128>, !adf.plio<In, 128>)
                  func.call @adf_cell0(%11, %10, %9, %8, %7, %6, %5, %4, %3, %2, %1, %0) {adf.cell} : (!adf.plio<In, 128>, !adf.plio<In, 128>, !adf.plio<In, 128>, !adf.plio<In, 128>, !adf.plio<Out, 128>, !adf.plio<In, 128>, !adf.plio<In, 128>, !adf.plio<Out, 128>, !adf.plio<In, 128>, !adf.plio<In, 128>, !adf.plio<Out, 128>, !adf.plio<Out, 128>) -> ()
                  adf.io.pop(%7, %arg2[%12, %15] [%c32, %c8] [%c1, %c1]) {type = f32} : (!adf.plio<Out, 128>, memref<64x16xi128>)
                  adf.io.pop(%4, %arg2[%12, %18] [%c32, %c8] [%c1, %c1]) {type = f32} : (!adf.plio<Out, 128>, memref<64x16xi128>)
                  adf.io.pop(%1, %arg2[%19, %15] [%c32, %c8] [%c1, %c1]) {type = f32} : (!adf.plio<Out, 128>, memref<64x16xi128>)
                  adf.io.pop(%0, %arg2[%19, %18] [%c32, %c8] [%c1, %c1]) {type = f32} : (!adf.plio<Out, 128>, memref<64x16xi128>)
                  adf.io.wait(%7) : !adf.plio<Out, 128>
                  adf.io.wait(%4) : !adf.plio<Out, 128>
                  adf.io.wait(%1) : !adf.plio<Out, 128>
                  adf.io.wait(%0) : !adf.plio<Out, 128>
                  adf.cell.launch.wait
                } {reduction}
              }
            }
          } {Array_Partition, reduction}
        }
      }
      adf.cell.launch.end
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
