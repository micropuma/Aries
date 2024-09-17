module {
  func.func @kernel_gemm0(%arg0: memref<32x32xi32, 2>, %arg1: memref<32x32xi32, 2>) -> memref<32x32xi32, 2> attributes {adf.kernel, edge_kernel} {
    %c8 = arith.constant 8 : index
    %c1 = arith.constant 1 : index
    %c32 = arith.constant 32 : index
    %c0 = arith.constant 0 : index
    %cst = arith.constant dense<0> : vector<8xi32>
    %c0_i32 = arith.constant 0 : i32
    %alloc = memref.alloc() : memref<32x32xi32, 2>
    scf.for %arg2 = %c0 to %c32 step %c1 {
      scf.for %arg3 = %c0 to %c32 step %c8 {
        vector.transfer_write %cst, %alloc[%arg2, %arg3] {in_bounds = [true]} : vector<8xi32>, memref<32x32xi32, 2>
      }
    }
    scf.for %arg2 = %c0 to %c32 step %c1 {
      scf.for %arg3 = %c0 to %c32 step %c8 {
        %0 = aievec.upd %alloc[%arg2, %arg3] {index = 0 : i8, offset = 0 : i32} : memref<32x32xi32, 2>, vector<8xi32>
        %1 = aievec.ups %0 {shift = 0 : i8} : vector<8xi32>, vector<8xi80>
        scf.for %arg4 = %c0 to %c32 step %c1 {
          %2 = aievec.upd %arg0[%arg2, %arg4] {index = 0 : i8, offset = 0 : i32} : memref<32x32xi32, 2>, vector<8xi32>
          %3 = aievec.upd %arg1[%arg4, %arg3] {index = 0 : i8, offset = 0 : i32} : memref<32x32xi32, 2>, vector<8xi32>
          %4 = aievec.concat %3, %3 : vector<8xi32>, vector<16xi32>
          %5 = aievec.mac %4, %2, %1 {xoffsets = "0x76543210", xstart = "0", zoffsets = "0x00000000", zstart = "0"} : vector<16xi32>, vector<8xi32>, vector<8xi80>
          %6 = aievec.srs %5, %c0_i32 : vector<8xi80>, i32, vector<8xi32>
          vector.transfer_write %6, %alloc[%arg2, %arg3] {in_bounds = [true]} : vector<8xi32>, memref<32x32xi32, 2>
        }
      }
    }
    return %alloc : memref<32x32xi32, 2>
  }
  func.func @kernel_gemm(%arg0: memref<32x32xi32, 2>, %arg1: memref<32x32xi32, 2>, %arg2: memref<32x32xi32, 2>) -> memref<32x32xi32, 2> attributes {adf.kernel} {
    %c8 = arith.constant 8 : index
    %c1 = arith.constant 1 : index
    %c32 = arith.constant 32 : index
    %c0 = arith.constant 0 : index
    %c0_i32 = arith.constant 0 : i32
    %alloc = memref.alloc() : memref<32x32xi32, 2>
    scf.for %arg3 = %c0 to %c32 step %c1 {
      scf.for %arg4 = %c0 to %c32 step %c8 {
        %0 = aievec.upd %arg0[%arg3, %arg4] {index = 0 : i8, offset = 0 : i32} : memref<32x32xi32, 2>, vector<8xi32>
        vector.transfer_write %0, %alloc[%arg3, %arg4] {in_bounds = [true]} : vector<8xi32>, memref<32x32xi32, 2>
        %1 = aievec.upd %alloc[%arg3, %arg4] {index = 0 : i8, offset = 0 : i32} : memref<32x32xi32, 2>, vector<8xi32>
        %2 = aievec.ups %1 {shift = 0 : i8} : vector<8xi32>, vector<8xi80>
        scf.for %arg5 = %c0 to %c32 step %c1 {
          %3 = aievec.upd %arg1[%arg3, %arg5] {index = 0 : i8, offset = 0 : i32} : memref<32x32xi32, 2>, vector<8xi32>
          %4 = aievec.upd %arg2[%arg5, %arg4] {index = 0 : i8, offset = 0 : i32} : memref<32x32xi32, 2>, vector<8xi32>
          %5 = aievec.concat %4, %4 : vector<8xi32>, vector<16xi32>
          %6 = aievec.mac %5, %3, %2 {xoffsets = "0x76543210", xstart = "0", zoffsets = "0x00000000", zstart = "0"} : vector<16xi32>, vector<8xi32>, vector<8xi80>
          %7 = aievec.srs %6, %c0_i32 : vector<8xi80>, i32, vector<8xi32>
          vector.transfer_write %7, %alloc[%arg3, %arg4] {in_bounds = [true]} : vector<8xi32>, memref<32x32xi32, 2>
        }
      }
    }
    return %alloc : memref<32x32xi32, 2>
  }
}

