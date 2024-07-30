FUNC:=gemm
PROJECT_NAME:=adf_graph_${FUNC}
KERNEL_NAME:=kernel_${FUNC}
PROJECT_PATH=./${PROJECT_NAME}
PROJECT_AIE_PATH=${PROJECT_PATH}/aie

all: fileCreate copyFile pass translate

fileCreate:
	mkdir -p ${PROJECT_AIE_PATH}

copyFile:
	cp -r template_files/Makefile_VCK190 ${PROJECT_PATH}/Makefile
	cp -r template_files/system.cfg ${PROJECT_PATH}

gemm: iomaterialize

tile:${PROJECT_PATH}/0_${FUNC}.tiled.mlir
${PROJECT_PATH}/0_${FUNC}.tiled.mlir: ${FUNC}.mlir
	aries-opt -o $@ $< \
			-aries-tiling="l1-tile-sizes=8,8,8 l2-tile-sizes=4,4,4"

exfunc: ${PROJECT_PATH}/1_${FUNC}.exfunc.mlir
${PROJECT_PATH}/1_${FUNC}.exfunc.mlir: ${PROJECT_PATH}/0_${FUNC}.tiled.mlir
	aries-opt -o $@ $< \
			-aries-func-extract

loopsim: ${PROJECT_PATH}/2_${FUNC}.loopsim.mlir
${PROJECT_PATH}/2_${FUNC}.loopsim.mlir: ${PROJECT_PATH}/1_${FUNC}.exfunc.mlir
	aries-opt -o $@ $< \
			-aries-loop-simplify

mem: ${PROJECT_PATH}/3_${FUNC}.memsubview.mlir
${PROJECT_PATH}/3_${FUNC}.memsubview.mlir: ${PROJECT_PATH}/2_${FUNC}.loopsim.mlir
	aries-opt -o $@ $< \
			-aries-mem-subview

hoistmem: ${PROJECT_PATH}/4_${FUNC}.hoistmem.mlir
${PROJECT_PATH}/4_${FUNC}.hoistmem.mlir: ${PROJECT_PATH}/3_${FUNC}.memsubview.mlir
	aries-opt -o $@ $< \
			-aries-mem-hoist

copymem: ${PROJECT_PATH}/5_${FUNC}.copymem.mlir
${PROJECT_PATH}/5_${FUNC}.copymem.mlir: ${PROJECT_PATH}/4_${FUNC}.hoistmem.mlir
	aries-opt -o $@ $< \
			-aries-mem-copy

toadf: ${PROJECT_PATH}/6_${FUNC}.adf.mlir
${PROJECT_PATH}/6_${FUNC}.adf.mlir: ${PROJECT_PATH}/5_${FUNC}.copymem.mlir
	aries-opt -o $@ $< \
			-aries-lower-to-adf \
			-canonicalize -cse

flow: ${PROJECT_PATH}/7_${FUNC}.flow.mlir
${PROJECT_PATH}/7_${FUNC}.flow.mlir: ${PROJECT_PATH}/6_${FUNC}.adf.mlir
	aries-opt -o $@ $< \
			-aries-dependency-extract

kernelio: ${PROJECT_PATH}/8_${FUNC}.kernelio.mlir
${PROJECT_PATH}/8_${FUNC}.kernelio.mlir: ${PROJECT_PATH}/7_${FUNC}.flow.mlir
	aries-opt -o $@ $< \
			-aries-kernel-interface-create

unrollfunc: ${PROJECT_PATH}/9_${FUNC}.unrollfunc.mlir
${PROJECT_PATH}/9_${FUNC}.unrollfunc.mlir: ${PROJECT_PATH}/8_${FUNC}.kernelio.mlir
	aries-opt -o $@ $< \
			-aries-func-unroll

dataforward: ${PROJECT_PATH}/10_${FUNC}.dataforward.mlir
${PROJECT_PATH}/10_${FUNC}.dataforward.mlir: ${PROJECT_PATH}/9_${FUNC}.unrollfunc.mlir
	aries-opt -o $@ $< \
			-aries-local-data-forward

dmaio: ${PROJECT_PATH}/11_${FUNC}.dmaio.mlir
${PROJECT_PATH}/11_${FUNC}.dmaio.mlir: ${PROJECT_PATH}/10_${FUNC}.dataforward.mlir
	aries-opt -o $@ $< \
			-aries-dma-to-io="port-type=GMIO" \
			-canonicalize -cse

adfgraph: ${PROJECT_PATH}/12_${FUNC}.adfgraph.mlir
${PROJECT_PATH}/12_${FUNC}.adfgraph.mlir: ${PROJECT_PATH}/11_${FUNC}.dmaio.mlir
	aries-opt -o $@ $< \
			-aries-adf-cell-create

iomaterialize: ${PROJECT_PATH}/13_${FUNC}.iomaterialize.mlir
${PROJECT_PATH}/13_${FUNC}.iomaterialize.mlir: ${PROJECT_PATH}/12_${FUNC}.adfgraph.mlir
	aries-opt -o $@ $< \
			-aries-io-materialize

pass: ${PROJECT_PATH}/${FUNC}.adf.mlir
${PROJECT_PATH}/${FUNC}.adf.mlir: ${FUNC}.mlir
	aries-opt -o $@ $< \
			-aries-tiling="l1-tile-sizes=32,32,32" \
			-aries-func-extract \
			-aries-loop-simplify \
			-aries-mem-subview \
			-aries-mem-hoist \
			-aries-mem-copy \
			-aries-lower-to-adf \
			-aries-dependency-extract \
			-aries-kernel-interface-create \
			-aries-func-unroll \
			-aries-local-data-forward \
			-aries-dma-to-io="port-type=GMIO plio-width=128 plio-freq=250" \
			-canonicalize -cse \
			-aries-adf-cell-create \
			-aries-io-materialize

translate: adf_graph kernel_header kernel_cc
adf_graph: adf_graph_temp
	aries-opt <${FUNC}.mlir \
		-aries-file-split="inputfile-name=adf_graph_temp.cpp path-name=${PROJECT_AIE_PATH}/" \
		>/dev/null
adf_graph_temp: ${PROJECT_AIE_PATH}/adf_graph_temp.cpp
${PROJECT_AIE_PATH}/adf_graph_temp.cpp: ${PROJECT_PATH}/${FUNC}.adf.mlir
	aries-translate -o $@ $< \
		-emit-aie-adf

kernel_split: ${PROJECT_PATH}/${KERNEL_NAME}.mlir
${PROJECT_PATH}/${KERNEL_NAME}.mlir: ${PROJECT_PATH}/${FUNC}.adf.mlir
	aries-opt -o $@ $< \
		-aries-kernel-split

kernel_vec: ${PROJECT_PATH}/auto_${KERNEL_NAME}_aie.mlir
${PROJECT_PATH}/auto_${KERNEL_NAME}_aie.mlir: ${PROJECT_PATH}/${KERNEL_NAME}_vector.mlir
	aries-opt -o $@ $< \
		--aie-vectorize \
		-unaligned-loads-check=false
${PROJECT_PATH}/${KERNEL_NAME}_vector.mlir: ${PROJECT_PATH}/${KERNEL_NAME}_unroll.mlir
	aries-opt -o $@ $< \
		--affine-super-vectorize="virtual-vector-size=8 vectorize-reductions" \
		--canonicalize
${PROJECT_PATH}/${KERNEL_NAME}_unroll.mlir: ${PROJECT_PATH}/${KERNEL_NAME}.mlir
	aries-opt -o $@ $< \
		--affine-loop-unroll="unroll-full unroll-full-threshold=3" \
		--canonicalize

kernel_header: ${PROJECT_AIE_PATH}/kernel.h
${PROJECT_AIE_PATH}/kernel.h: ${PROJECT_PATH}/${KERNEL_NAME}.mlir
	aries-translate -o $@ $< \
		-emit-kenrel-header

kernel_cc: ${PROJECT_AIE_PATH}/${KERNEL_NAME}.cc
${PROJECT_AIE_PATH}/${KERNEL_NAME}.cc: ${PROJECT_PATH}/auto_${KERNEL_NAME}_aie.mlir
	aries-translate -o $@ $< \
		-emit-kenrel-func

clean_test:
	rm -rf ${PROJECT_PATH}/*_${FUNC}.*

clean_translate:
	rm -rf ${PROJECT_AIE_PATH}/adf_graph.cpp ${PROJECT_AIE_PATH}/kernel.h ${PROJECT_AIE_PATH}/${KERNEL_NAME}.cc

clean_pass:
	rm -rf ${PROJECT_PATH}/${FUNC}.adf.mlir

clean:
	rm -rf ${PROJECT_PATH}