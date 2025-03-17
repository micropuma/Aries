
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

const int h1=32; //i
const int w2=32; //j
const int w1=32; //k
const int A_SIZE=h1*w1;
const int B_SIZE=w1*w2;
const int C_SIZE=h1*w2;
const int boundary_i=h1/2;
const int boundary_j=w2/8;
const int boundary_k=w1/8-1;
const int judge_j = boundary_j-1;
const int lhs_jump0=w1-4;
const int lhs_jump1=2*w1-4;
const int lhs_jump2=-4;
const int rhs_jump0=(w1-1)*w2-8;
const int rhs_jump1=w1*w2-8;
const int out_jump0=w2-8;
const int out_jump1=-8;

// Assumes all the operands are row-major
// The basic block is 2*8*8 (i, j, k)
void kernel_gemm(input_buffer<float, adf::extents<A_SIZE>>& __restrict in0, input_buffer<float, adf::extents<B_SIZE>>& __restrict in1, input_buffer<float, adf::extents<C_SIZE>>& __restrict in2, output_buffer<float, adf::extents<C_SIZE>>& __restrict out0) {
  float * __restrict lhs = (float *)in0.data();
  float * __restrict rhs = (float *)in1.data();
  float * __restrict accin = (float *)in2.data();
  float * __restrict out = (float *)out0.data();

  aie::vector<float, 16> lhs_v16 = undef_v16float();
  aie::vector<float, 16> rhs_v16 = undef_v16float();

  for (unsigned int i=0;i<boundary_i;i++) 
	chess_prepare_for_pipelining
	chess_loop_range(boundary_i,boundary_i){
    for (unsigned int j=0; j< boundary_j; j++)
		chess_prepare_for_pipelining
		chess_loop_range(boundary_j,boundary_j){
      
      int jump_lhs = lhs_jump1;
      int jump_rhs = rhs_jump0;
      int jump_out = out_jump0;
			if (j == judge_j){
        jump_lhs = lhs_jump2;
        jump_rhs = rhs_jump1;
        jump_out = out_jump1;
      }else{
        jump_lhs = lhs_jump1;
        jump_rhs = rhs_jump0;
        jump_out = out_jump0;
      }
      aie::vector<float,8> acc0 = aie::load_v<8>(accin);
      accin += w2;
			aie::vector<float,8> acc1 = aie::load_v<8>(accin);
      accin -= jump_out;
      for (unsigned int k=0;k<boundary_k;k++)
			chess_prepare_for_pipelining
			chess_loop_range(boundary_k,boundary_k)
			{ 
        //////////// Vector Preload
        lhs_v16 = lhs_v16.insert(0, aie::load_v<4>(lhs));
        lhs += w1;
        rhs_v16 = rhs_v16.insert(0, aie::load_v<8>(rhs));
        rhs += w2;
        lhs_v16 = lhs_v16.insert(1, aie::load_v<4>(lhs));
        lhs -= lhs_jump0;

        ///////////// Calculate first 2*4*8
        acc0 = fpmac(acc0, rhs_v16, 0, 0x76543210, lhs_v16.extract<8>(0), 0, 0x0);
        lhs_v16 = lhs_v16.insert(2, aie::load_v<4>(lhs));
        lhs += w1;
        rhs_v16 = rhs_v16.insert(1, aie::load_v<8>(rhs));
        rhs += w2;
        acc1 = fpmac(acc1, rhs_v16, 0, 0x76543210, lhs_v16.extract<8>(0), 4, 0x0);
        lhs_v16 = lhs_v16.insert(3, aie::load_v<4>(lhs));
        lhs -= lhs_jump0;

        acc0 = fpmac(acc0, rhs_v16, 8, 0x76543210, lhs_v16.extract<8>(0), 1, 0x0);
        rhs_v16 = rhs_v16.insert(0, aie::load_v<8>(rhs));
        rhs += w2;
        acc1 = fpmac(acc1, rhs_v16, 8, 0x76543210, lhs_v16.extract<8>(0), 5, 0x0);

        acc0 = fpmac(acc0, rhs_v16, 0, 0x76543210, lhs_v16.extract<8>(0), 2, 0x0);
        rhs_v16 = rhs_v16.insert(1, aie::load_v<8>(rhs));
        rhs += w2;
        acc1 = fpmac(acc1, rhs_v16, 0, 0x76543210, lhs_v16.extract<8>(0), 6, 0x0);

        acc0 = fpmac(acc0, rhs_v16, 8, 0x76543210, lhs_v16.extract<8>(0), 3, 0x0);
        rhs_v16 = rhs_v16.insert(0, aie::load_v<8>(rhs));
        rhs += w2;
        acc1 = fpmac(acc1, rhs_v16, 8, 0x76543210, lhs_v16.extract<8>(0), 7, 0x0);


        //////////// Calculate second 2*4*8
        acc0 = fpmac(acc0, rhs_v16, 0, 0x76543210, lhs_v16.extract<8>(1), 0, 0x0);
        rhs_v16 = rhs_v16.insert(1, aie::load_v<8>(rhs));
        rhs += w2;
        acc1 = fpmac(acc1, rhs_v16, 0, 0x76543210, lhs_v16.extract<8>(1), 4, 0x0);

        acc0 = fpmac(acc0, rhs_v16, 8, 0x76543210, lhs_v16.extract<8>(1), 1, 0x0);
        rhs_v16 = rhs_v16.insert(0, aie::load_v<8>(rhs));
        rhs += w2;
        acc1 = fpmac(acc1, rhs_v16, 8, 0x76543210, lhs_v16.extract<8>(1), 5, 0x0);

        acc0 = fpmac(acc0, rhs_v16, 0, 0x76543210, lhs_v16.extract<8>(1), 2, 0x0);
        rhs_v16 = rhs_v16.insert(1, aie::load_v<8>(rhs));
        rhs += w2;
        acc1 = fpmac(acc1, rhs_v16, 0, 0x76543210, lhs_v16.extract<8>(1), 6, 0x0);

        acc0 = fpmac(acc0, rhs_v16, 8, 0x76543210, lhs_v16.extract<8>(1), 3, 0x0);
        acc1 = fpmac(acc1, rhs_v16, 8, 0x76543210, lhs_v16.extract<8>(1), 7, 0x0);
      }
      // Last reduction iter
      lhs_v16 = lhs_v16.insert(0, aie::load_v<4>(lhs));
      lhs += w1;
      rhs_v16 = rhs_v16.insert(0, aie::load_v<8>(rhs));
      rhs += w2;
      lhs_v16 = lhs_v16.insert(1, aie::load_v<4>(lhs));
      lhs -= lhs_jump0;

      ///////////// Calculate first 2*4*8
      acc0 = fpmac(acc0, rhs_v16, 0, 0x76543210, lhs_v16.extract<8>(0), 0, 0x0);
      lhs_v16 = lhs_v16.insert(2, aie::load_v<4>(lhs));
      lhs += w1;
      rhs_v16 = rhs_v16.insert(1, aie::load_v<8>(rhs));
      rhs += w2;
      acc1 = fpmac(acc1, rhs_v16, 0, 0x76543210, lhs_v16.extract<8>(0), 4, 0x0);
      lhs_v16 = lhs_v16.insert(3, aie::load_v<4>(lhs));
      lhs -= jump_lhs;

      acc0 = fpmac(acc0, rhs_v16, 8, 0x76543210, lhs_v16.extract<8>(0), 1, 0x0);
      rhs_v16 = rhs_v16.insert(0, aie::load_v<8>(rhs));
      rhs += w2;
      acc1 = fpmac(acc1, rhs_v16, 8, 0x76543210, lhs_v16.extract<8>(0), 5, 0x0);

      acc0 = fpmac(acc0, rhs_v16, 0, 0x76543210, lhs_v16.extract<8>(0), 2, 0x0);
      rhs_v16 = rhs_v16.insert(1, aie::load_v<8>(rhs));
      rhs += w2;
      acc1 = fpmac(acc1, rhs_v16, 0, 0x76543210, lhs_v16.extract<8>(0), 6, 0x0);

      acc0 = fpmac(acc0, rhs_v16, 8, 0x76543210, lhs_v16.extract<8>(0), 3, 0x0);
      rhs_v16 = rhs_v16.insert(0, aie::load_v<8>(rhs));
      rhs += w2;
      acc1 = fpmac(acc1, rhs_v16, 8, 0x76543210, lhs_v16.extract<8>(0), 7, 0x0);


      //////////// Calculate second 2*4*8
      acc0 = fpmac(acc0, rhs_v16, 0, 0x76543210, lhs_v16.extract<8>(1), 0, 0x0);
      rhs_v16 = rhs_v16.insert(1, aie::load_v<8>(rhs));
      rhs += w2;
      acc1 = fpmac(acc1, rhs_v16, 0, 0x76543210, lhs_v16.extract<8>(1), 4, 0x0);

      acc0 = fpmac(acc0, rhs_v16, 8, 0x76543210, lhs_v16.extract<8>(1), 1, 0x0);
      rhs_v16 = rhs_v16.insert(0, aie::load_v<8>(rhs));
      rhs += w2;
      acc1 = fpmac(acc1, rhs_v16, 8, 0x76543210, lhs_v16.extract<8>(1), 5, 0x0);

      acc0 = fpmac(acc0, rhs_v16, 0, 0x76543210, lhs_v16.extract<8>(1), 2, 0x0);
      rhs_v16 = rhs_v16.insert(1, aie::load_v<8>(rhs));
      rhs -= jump_rhs;
      acc1 = fpmac(acc1, rhs_v16, 0, 0x76543210, lhs_v16.extract<8>(1), 6, 0x0);

      acc0 = fpmac(acc0, rhs_v16, 8, 0x76543210, lhs_v16.extract<8>(1), 3, 0x0);
      aie::store_v(out, acc0);
      out += w2;
      acc1 = fpmac(acc1, rhs_v16, 8, 0x76543210, lhs_v16.extract<8>(1), 7, 0x0);
      aie::store_v(out, acc1);
      out -= jump_out;
    }
  }
  return;
}