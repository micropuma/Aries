gemm: tile exfunc loopsim mem hoistmem copymem unrollfunc

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
			-aries-mem-subview \
			-canonicalize -cse

hoistmem: 4_gemm.hoistmem.mlir
4_gemm.hoistmem.mlir: 3_gemm.memsubview.mlir
	aries-opt -o $@ $< \
			-aries-mem-hoist

copymem: 5_gemm.copymem.mlir
5_gemm.copymem.mlir: 4_gemm.hoistmem.mlir
	aries-opt -o $@ $< \
			-mlir-print-ir-after-failure \
			-aries-mem-copy

unrollfunc: 6_gemm.unrollfunc.mlir
6_gemm.unrollfunc.mlir: 5_gemm.copymem.mlir
	aries-opt -o $@ $< \
			-aries-func-unroll

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