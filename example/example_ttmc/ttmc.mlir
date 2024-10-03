// D(i, j, k)+ = A(i, l, m) * B(l, j) * C(m, k)

// i = 4, j = 512, k = 768, l = 1024, m = 4096,
module {
  func.func @ttmc(%A: memref<4x1024x4096xi32>, %B: memref<1024x512xi32>, %C: memref<4096x768xi32>, %D: memref<4x512x768xi32>) {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 512 {
        affine.for %k = 0 to 768 {
          affine.for %l = 0 to 1024 {
            affine.for %m = 0 to 4096 {
              %0 = affine.load %A[%i, %l, %m] : memref<4x1024x4096xi32>
              %1 = affine.load %B[%l, %j] : memref<1024x512xi32>
              %2 = affine.load %C[%m, %k] : memref<4096x768xi32>
              %3 = arith.muli %0, %1 : i32
              %4 = arith.muli %3, %2 : i32
              %5 = affine.load %D[%i, %j, %k] : memref<4x512x768xi32>
              %6 = arith.addi %5, %4 : i32
              affine.store %6, %D[%i, %j, %k] : memref<4x512x768xi32>
            } {reduction}
          }{reduction}
        }
      }
    }
    return
  }
}

// i = 8, j = 128, k = 128, l = 128, m = 128,
// module {
//   func.func @ttmc(%A: memref<8x128x128xi32>, %B: memref<128x128xi32>, %C: memref<128x128xi32>, %D: memref<4x128x128xi32>) {
//     affine.for %i = 0 to 8 {
//       affine.for %j = 0 to 128 {
//         affine.for %k = 0 to 128 {
//           affine.for %l = 0 to 128 {
//             affine.for %m = 0 to 128 {
//               %0 = affine.load %A[%i, %l, %m] : memref<8x128x128xi32>
//               %1 = affine.load %B[%l, %j] : memref<128x128xi32>
//               %2 = affine.load %C[%m, %k] : memref<128x128xi32>
//               %3 = arith.muli %0, %1 : i32
//               %4 = arith.muli %3, %2 : i32
//               %5 = affine.load %D[%i, %j, %k] : memref<4x128x128xi32>
//               %6 = arith.addi %5, %4 : i32
//               affine.store %6, %D[%i, %j, %k] : memref<4x128x128xi32>
//             } {reduction}
//           }{reduction}
//         }
//       }
//     }
//     return
//   }
// }