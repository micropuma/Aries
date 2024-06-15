gemm: toadf

tile:0_gemm.tiled.mlir
0_gemm.tiled.mlir: gemm.mlir
	aries-opt -o $@ $< \
			-aries-tiling="tile-sizes=32,32,16"

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
			-aries-mem-copy

flow: 6_gemm.flow.mlir
6_gemm.flow.mlir: 5_gemm.copymem.mlir
	aries-opt -o $@ $< \
			-aries-dependency-extract


unrollfunc: 7_gemm.unrollfunc.mlir
7_gemm.unrollfunc.mlir: 6_gemm.flow.mlir
	aries-opt -o $@ $< \
			-aries-func-unroll \
			-canonicalize -cse

toadf: 8_gemm.adf.mlir
8_gemm.adf.mlir: 7_gemm.unrollfunc.mlir
	aries-opt -o $@ $< \
			-aries-lower-to-adf \
			-mlir-print-ir-after-failure \
			-canonicalize -cse

pass:0_gemm.adf.mlir
0_gemm.adf.mlir: gemm.mlir
	aries-opt -o $@ $< \
			-aries-tiling="tile-sizes=32,32,32" \
			-aries-func-extract \
			-aries-loop-simplify \
			-aries-mem-subview \
			-aries-mem-hoist \
			-aries-mem-copy \
			-aries-dependency-extract \
			-aries-func-unroll \
			-aries-lower-to-adf \
			-canonicalize -cse


add:0_add.mlir
0_add.mlir: add.mlir
	aries-opt -o $@ $< \
			-aries-tiling="tile-sizes=32" \
			-aries-func-extract \
			-aries-loop-simplify \
			-aries-mem-subview \
			-aries-mem-hoist \
			-aries-mem-copy

test: test_out.mlir
test_out.mlir: test_in.mlir
	aries-opt -o $@ $< \
		-aries-adf-test

convert: convert_out.mlir
convert_out.mlir: 5_gemm.copymem.mlir
	aries-opt -o $@ $< \
			-aries-lower-to-adf \
			-canonicalize -cse

clean_gemm:
	rm -rf *_gemm*.mlir

clean_add:
	rm -rf *_add*.mlir

clean_test:
	rm -rf test_out.mlir

clean_convert:
	rm -rf convert_out.mlir