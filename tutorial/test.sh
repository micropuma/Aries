#!/bin/bash

# 配置参数
FUNC="forward"
MLIR="test"
PROJECT_PATH="./"
PLIOWidth=128
AXIWidth=512
ColNum=50
RowNum=8
ColOffset=0
RowOffset=0
ColGap=0
FirstCol=6
NumShim=39
MidLine=24
ChalIn=3
ChalOut=3
CoreAlgo=2
EnableIOCons="true"
Link_file=1

# 平铺参数
L1_0=1; L2_0=1
L1_1=8; L2_1=2
L1_2=12; L2_2=2
L1_3=1; L2_3=8
L1_4=2; L2_4=8

L1_Tiling="${L1_0},${L1_1},${L1_2},${L1_3},${L1_4}"
L2_Tiling="${L2_0},${L2_1},${L2_2},${L2_3},${L2_4}"
EN_Tiling="true"

# 缓冲配置
BUFF_0=0; BUFF_1=1; BUFF_2=0; BUFF_3=1
buf_sel="${BUFF_0},${BUFF_1},${BUFF_2},${BUFF_3}"

# 功能开关
EN_PL="true"
EN_AIE2="false" 
EN_SERIAL="false"

# 确定IO类型
if [ "$EN_AIE2" = "true" ]; then
    IOType="AIE2_GMIO"
elif [ "$EN_PL" = "true" ]; then
    IOType="PLIO"
else
    IOType="GMIO"
fi

# 构建参数选项
ARIES_OPTIONS=(
    "tile-func-name=${FUNC}"
    "l1-tile-sizes=${L1_Tiling}"
    "l2-tile-sizes=${L2_Tiling}" 
    "en-newtiling=${EN_Tiling}"
    "port-type=${IOType}"
    "plio-width=${PLIOWidth}"
    "en-pl=${EN_PL}"
    "en-aie2=${EN_AIE2}"
    "col-num=${ColNum}"
    "row-num=${RowNum}"
    "col-offset=${ColOffset}"
    "row-offset=${RowOffset}"
    "core-gap=${ColGap}"
    "core-algo=${CoreAlgo}"
    "first-col=${FirstCol}"
    "num-shim=${NumShim}"
    "mid-line=${MidLine}"
    "chal-in=${ChalIn}"
    "chal-out=${ChalOut}"
    "iocons=${EnableIOCons}"
    "buf-sels=${buf_sel}"
    "axi-width=${AXIWidth}"
    "en-serial=${EN_SERIAL}"
)

# 输入输出文件
input_file="${MLIR}.mlir"
output_file="${PROJECT_PATH}/${FUNC}.adf.mlir"
log_file="${PROJECT_PATH}/pass.log"

# 执行转换命令
echo "Generating ${output_file}..."
aries-opt -o "$output_file" "$input_file" \
    -aries-pipeline="${ARIES_OPTIONS[*]}" \
    -mlir-print-ir-after-all 2>&1 | tee "$log_file"

# 检查执行结果
if [ $? -eq 0 ]; then
    echo "Successfully generated AIE dataflow MLIR"
else
    echo "Error occurred during compilation" >&2
    exit 1
fi