tile:0_gemm.tiled.mlir
0_gemm.tiled.mlir: gemm.mlir
	aries-opt -o $@ $< \
			-aries-tiling="tile-sizes=32,32,32"

exfunc: 1_gemm.exfunc.mlir
1_gemm.exfunc.mlir: 0_gemm.tiled.mlir
	aries-opt -o $@ $< \
			-aries-func-extract

tadd:0_add.tiled.mlir
0_add.tiled.mlir: add.mlir
	aries-opt -o $@ $< \
			-aries-tiling="tile-sizes=32"

exadd: 1_add.exfunc.mlir
1_add.exfunc.mlir: 0_add.tiled.mlir
	aries-opt -o $@ $< \
			-aries-func-extract