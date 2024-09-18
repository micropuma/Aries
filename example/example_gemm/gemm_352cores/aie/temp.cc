//_aries_split_//kernel_gemm0.cc//_aries_split_//
//===------------------------------------------------------------*- C++ -*-===//
//
// Automatically generated file for AIE kernel supported by Vitis Flow.
//
//===----------------------------------------------------------------------===//

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include <aie_api/utils.hpp>
#include <adf/io_buffer/io_buffer.h>
using namespace adf;
static int iteration = 0;
void kernel_gemm0(input_buffer<int32_t, adf::extents<1024>>&  __restrict in0, input_buffer<int32_t, adf::extents<1024>>&  __restrict in1, output_buffer<int32_t, adf::extents<1024>>&  __restrict out0) {
  int32_t * __restrict v1 = (int32_t *)in0.data();
  int32_t * __restrict v2 = (int32_t *)in1.data();
  int32_t * __restrict v3 = (int32_t *)out0.data();
  size_t v4 = 8;
  size_t v5 = 1;
  size_t v6 = 32;
  size_t v7 = 0;
  aie::vector<int32,8> v8 = null_v8int32();
  int32_t v9 = 0;
  for (size_t v10 = v7; v10 < v6; v10 += v5)
  chess_prepare_for_pipelining
  chess_loop_range(32, 32)
  {
    for (size_t v11 = v7; v11 < v6; v11 += v4)
    chess_prepare_for_pipelining
    chess_loop_range(4, 4)
    {
      aie::store_v(v3+32*v10+v11,v8);
    }
  }
  for (size_t v12 = v7; v12 < v6; v12 += v5)
  chess_prepare_for_pipelining
  chess_loop_range(32, 32)
  {
    for (size_t v13 = v7; v13 < v6; v13 += v4)
    chess_prepare_for_pipelining
    chess_loop_range(4, 4)
    {
      aie::vector<int32,8> v14 = aie::load_v<8>(v3+32*v12+v13);
      aie::accum<acc80,8> v15 = lups(v14, 0);
      for (size_t v16 = v7; v16 < v6; v16 += v5)
      chess_prepare_for_pipelining
      chess_loop_range(32, 32)
      {
        aie::vector<int32,8> v17 = aie::load_v<8>(v1+32*v12+v16);
        aie::vector<int32,8> v18 = aie::load_v<8>(v2+32*v16+v13);
        aie::vector<int32,16> v19 = concat(v18, v18);
        v15 = lmac8(v15, v19, 0, 0x76543210, v17, 0, 0x00000000);
        aie::vector<int32,8> v20 = srs(v15, v9);
        aie::store_v(v3+32*v12+v13,v20);
      }
    }
  }
  return;
}

//_aries_split_//kernel_gemm.cc//_aries_split_//
//===------------------------------------------------------------*- C++ -*-===//
//
// Automatically generated file for AIE kernel supported by Vitis Flow.
//
//===----------------------------------------------------------------------===//

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include <aie_api/utils.hpp>
#include <adf/io_buffer/io_buffer.h>
using namespace adf;
static int iteration = 0;
void kernel_gemm(input_buffer<int32_t, adf::extents<1024>>&  __restrict in0, input_buffer<int32_t, adf::extents<1024>>&  __restrict in1, input_buffer<int32_t, adf::extents<1024>>&  __restrict in2, output_buffer<int32_t, adf::extents<1024>>&  __restrict out0) {
  int32_t * __restrict v1 = (int32_t *)in0.data();
  int32_t * __restrict v2 = (int32_t *)in1.data();
  int32_t * __restrict v3 = (int32_t *)in2.data();
  int32_t * __restrict v4 = (int32_t *)out0.data();
  size_t v5 = 8;
  size_t v6 = 1;
  size_t v7 = 32;
  size_t v8 = 0;
  int32_t v9 = 0;
  for (size_t v10 = v8; v10 < v7; v10 += v6)
  chess_prepare_for_pipelining
  chess_loop_range(32, 32)
  {
    for (size_t v11 = v8; v11 < v7; v11 += v5)
    chess_prepare_for_pipelining
    chess_loop_range(4, 4)
    {
      aie::vector<int32,8> v12 = aie::load_v<8>(v1+32*v10+v11);
      aie::store_v(v4+32*v10+v11,v12);
      aie::vector<int32,8> v13 = aie::load_v<8>(v4+32*v10+v11);
      aie::accum<acc80,8> v14 = lups(v13, 0);
      for (size_t v15 = v8; v15 < v7; v15 += v6)
      chess_prepare_for_pipelining
      chess_loop_range(32, 32)
      {
        aie::vector<int32,8> v16 = aie::load_v<8>(v2+32*v10+v15);
        aie::vector<int32,8> v17 = aie::load_v<8>(v3+32*v15+v11);
        aie::vector<int32,16> v18 = concat(v17, v17);
        v14 = lmac8(v14, v18, 0, 0x76543210, v16, 0, 0x00000000);
        aie::vector<int32,8> v19 = srs(v14, v9);
        aie::store_v(v4+32*v10+v11,v19);
      }
    }
  }
  return;
}

