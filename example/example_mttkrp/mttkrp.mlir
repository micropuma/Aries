// D(i, j)+ = A(i, k, l) * B(k, j) * C(l, j)
// i = 64, j = 6144, k = 768, l = 2048
// module {
//   func.func @mttkrp(%A: memref<64x768x2048xi32>, %B: memref<768x6144xi32>, %C: memref<2048x6144xi32>, %D: memref<64x6144xi32>) {
//     affine.for %i = 0 to 64 {
//       affine.for %j = 0 to 6144 {
//         affine.for %k = 0 to 768 {
//           affine.for %l = 0 to 2048 {
//             %0 = affine.load %A[%i, %k, %l] : memref<64x768x2048xi32>
//             %1 = affine.load %B[%k, %j] : memref<768x6144xi32>
//             %2 = affine.load %C[%l, %j] : memref<2048x6144xi32>
//             %3 = arith.muli %0, %1 : i32
//             %4 = arith.muli %3, %2 : i32
//             %5 = affine.load %D[%i, %j] : memref<64x6144xi32>
//             %6 = arith.addi %5, %4 : i32
//             affine.store %6, %D[%i, %j] : memref<64x6144xi32>
//           } {reduction}
//         }{reduction}
//       }
//     }
//     return
//   }
// }



// i = 60, j = 2048, k = 384, l = 4096
// module {
//   func.func @mttkrp(%A: memref<60x384x4096xi32>, %B: memref<384x2048xi32>, %C: memref<4096x2048xi32>, %D: memref<60x2048xi32>) {
//     affine.for %i = 0 to 60 {
//       affine.for %j = 0 to 2048 {
//         affine.for %k = 0 to 384 {
//           affine.for %l = 0 to 4096 {
//             %0 = affine.load %A[%i, %k, %l] : memref<60x384x4096xi32>
//             %1 = affine.load %B[%k, %j] : memref<384x2048xi32>
//             %2 = affine.load %C[%l, %j] : memref<4096x2048xi32>
//             %3 = arith.muli %0, %1 : i32
//             %4 = arith.muli %3, %2 : i32
//             %5 = affine.load %D[%i, %j] : memref<60x2048xi32>
//             %6 = arith.addi %5, %4 : i32
//             affine.store %6, %D[%i, %j] : memref<60x2048xi32>
//           } {reduction}
//         }{reduction}
//       }
//     }
//     return
//   }
// }



// i = 60, j = 6144, k = 1024, l = 1024
// module {
//   func.func @mttkrp(%A: memref<60x1024x1024xi32>, %B: memref<1024x6144xi32>, %C: memref<1024x6144xi32>, %D: memref<60x6144xi32>) {
//     affine.for %i = 0 to 60 {
//       affine.for %j = 0 to 6144 {
//         affine.for %k = 0 to 1024 {
//           affine.for %l = 0 to 1024 {
//             %0 = affine.load %A[%i, %k, %l] : memref<60x1024x1024xi32>
//             %1 = affine.load %B[%k, %j] : memref<1024x6144xi32>
//             %2 = affine.load %C[%l, %j] : memref<1024x6144xi32>
//             %3 = arith.muli %0, %1 : i32
//             %4 = arith.muli %3, %2 : i32
//             %5 = affine.load %D[%i, %j] : memref<60x6144xi32>
//             %6 = arith.addi %5, %4 : i32
//             affine.store %6, %D[%i, %j] : memref<60x6144xi32>
//           } {reduction}
//         }{reduction}
//       }
//     }
//     return
//   }
// }

// i = 40, j = 4608, k = 1024, l = 1024
// module {
//   func.func @mttkrp(%A: memref<40x1024x1024xi32>, %B: memref<1024x4608xi32>, %C: memref<1024x4608xi32>, %D: memref<40x4608xi32>) {
//     affine.for %i = 0 to 40 {
//       affine.for %j = 0 to 4608 {
//         affine.for %k = 0 to 1024 {
//           affine.for %l = 0 to 1024 {
//             %0 = affine.load %A[%i, %k, %l] : memref<40x1024x1024xi32>
//             %1 = affine.load %B[%k, %j] : memref<1024x4608xi32>
//             %2 = affine.load %C[%l, %j] : memref<1024x4608xi32>
//             %3 = arith.muli %0, %1 : i32
//             %4 = arith.muli %3, %2 : i32
//             %5 = affine.load %D[%i, %j] : memref<40x4608xi32>
//             %6 = arith.addi %5, %4 : i32
//             affine.store %6, %D[%i, %j] : memref<40x4608xi32>
//           } {reduction}
//         }{reduction}
//       }
//     }
//     return
//   }
// }

// i = 8, j = 256, k = 256, l = 128
module {
  func.func @mttkrp(%A: memref<8x256x128xi32>, %B: memref<256x256xi32>, %C: memref<128x256xi32>, %D: memref<8x256xi32>) {
    affine.for %i = 0 to 8 {
      affine.for %j = 0 to 256 {
        affine.for %k = 0 to 256 {
          affine.for %l = 0 to 128 {
            %0 = affine.load %A[%i, %k, %l] : memref<8x256x128xi32>
            %1 = affine.load %B[%k, %j] : memref<256x256xi32>
            %2 = affine.load %C[%l, %j] : memref<128x256xi32>
            %3 = arith.muli %0, %1 : i32
            %4 = arith.muli %3, %2 : i32
            %5 = affine.load %D[%i, %j] : memref<8x256xi32>
            %6 = arith.addi %5, %4 : i32
            affine.store %6, %D[%i, %j] : memref<8x256xi32>
          } {reduction}
        }{reduction}
      }
    }
    return
  }
}

// i = 30, j = 4608, k = 2048, l = 1280
// module {
//   func.func @mttkrp(%A: memref<30x2048x1280xi32>, %B: memref<2048x4608xi32>, %C: memref<1280x4608xi32>, %D: memref<30x4608xi32>) {
//     affine.for %i = 0 to 30 {
//       affine.for %j = 0 to 4608 {
//         affine.for %k = 0 to 2048 {
//           affine.for %l = 0 to 1280 {
//             %0 = affine.load %A[%i, %k, %l] : memref<30x2048x1280xi32>
//             %1 = affine.load %B[%k, %j] : memref<2048x4608xi32>
//             %2 = affine.load %C[%l, %j] : memref<1280x4608xi32>
//             %3 = arith.muli %0, %1 : i32
//             %4 = arith.muli %3, %2 : i32
//             %5 = affine.load %D[%i, %j] : memref<30x4608xi32>
//             %6 = arith.addi %5, %4 : i32
//             affine.store %6, %D[%i, %j] : memref<30x4608xi32>
//           } {reduction}
//         }{reduction}
//       }
//     }
//     return
//   }
// }