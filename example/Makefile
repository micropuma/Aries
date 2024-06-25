gemm: adfgraph

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
			-aries-mem-copy

toadf: 6_gemm.adf.mlir
6_gemm.adf.mlir: 5_gemm.copymem.mlir
	aries-opt -o $@ $< \
			-aries-lower-to-adf \
			-canonicalize -cse

flow: 7_gemm.flow.mlir
7_gemm.flow.mlir: 6_gemm.adf.mlir
	aries-opt -o $@ $< \
			-aries-dependency-extract

kernelio: 8_gemm.kernelio.mlir
8_gemm.kernelio.mlir: 7_gemm.flow.mlir
	aries-opt -o $@ $< \
			-aries-kernel-interface-create

unrollfunc: 9_gemm.unrollfunc.mlir
9_gemm.unrollfunc.mlir: 8_gemm.kernelio.mlir
	aries-opt -o $@ $< \
			-aries-func-unroll

dataforward: 10_gemm.dataforward.mlir
10_gemm.dataforward.mlir: 9_gemm.unrollfunc.mlir
	aries-opt -o $@ $< \
			-aries-local-data-forward

dmaio: 11_gemm.dmaio.mlir
11_gemm.dmaio.mlir: 10_gemm.dataforward.mlir
	aries-opt -o $@ $< \
			-aries-dma-to-io="port-type=PLIO port-width=128" \
			-canonicalize -cse

adfgraph: 12_gemm.adfgraph.mlir
12_gemm.adfgraph.mlir: 11_gemm.dmaio.mlir
	aries-opt -o $@ $< \
			-aries-adf-graph-create

pass:0_gemm.adf.mlir
0_gemm.adf.mlir: gemm.mlir
	aries-opt -o $@ $< \
			-aries-tiling="tile-sizes=32,32,32" \
			-aries-func-extract \
			-aries-loop-simplify \
			-aries-mem-subview \
			-aries-mem-hoist \
			-aries-mem-copy \
			-aries-lower-to-adf \
			-aries-dependency-extract \
			-aries-func-unroll \
			-aries-local-data-forward \
			-aries-dma-to-io="port-type=PLIO port-width=128" \
			-aries-adf-graph-create \
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