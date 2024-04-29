gemm: tile exfunc loopsim mem unrollfunc

tile:0_gemm.tiled.mlir
0_gemm.tiled.mlir: gemm.mlir
	aries-opt -o $@ $< \
			-aries-tiling="tile-sizes=32,32,32"

exfunc: 1_gemm.exfunc.mlir
1_gemm.exfunc.mlir: 0_gemm.tiled.mlir
	aries-opt -o $@ $< \
			-aries-func-extract

loopsim: 2_gemm.loopsim.mlir
2_gemm.loopsim.mlir: 1_gemm.exfunc.mlir
	aries-opt -o $@ $< \
			-aries-loop-simplify

mem: 3_gemm.memsubview.mlir
3_gemm.memsubview.mlir: 2_gemm.loopsim.mlir
	aries-opt -o $@ $< \
			-mlir-print-ir-after-failure \
			-aries-memref-subview \
			-canonicalize -cse

unrollfunc: 4_gemm.unrollfunc.mlir
4_gemm.unrollfunc.mlir: 3_gemm.memsubview.mlir
	aries-opt -o $@ $< \
			-aries-func-unroll \
			-canonicalize -cse

hoistmem: 5_gemm.hoistmem.mlir
5_gemm.hoistmem.mlir: 4_gemm.unrollfunc.mlir
	aries-opt -o $@ $< \
			-aries-mem-hoist

tadd:0_add.tiled.mlir
0_add.tiled.mlir: add.mlir
	aries-opt -o $@ $< \
			-aries-tiling="tile-sizes=32"

exadd: 1_add.exfunc.mlir
1_add.exfunc.mlir: 0_add.tiled.mlir
	aries-opt -o $@ $< \
			-aries-func-extract

clean_gemm:
	rm -rf *_gemm*.mlir