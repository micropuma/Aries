import os
import sys
import numpy as np
cur_dir = os.path.dirname(os.path.abspath(__file__))
aries_path = cur_dir + "/../../../"
sys.path.append(aries_path)
from newfrontend import *

# # 一维ReLU核心计算kernel
# @task_kernel(external_path="kernel_relu_1d_v0", para=[32])
# def kernel_relu_1d(
#     TileIn: float32[32],   # 输入向量分块
#     TileOut: float32[32]   # 输出向量分块
# ):
#     # ReLU计算：f(x) = max(0, x)
#     for i in range(0, 32): 
#         TileOut[i] = TileIn[i] if TileIn[i] > 0.0 else 0.0

# 一维向量分块处理
@task_tile(False)
def relu_tile_1d(X: float32[64], Y: float32[64], **kwargs):
    i, _, _ = aries.tile_ranks(**kwargs)  # 只需要i维度
    TI, _, _ = aries.tile_sizes(**kwargs)  # 只需要TI

    ti = aries.arange(i*TI, (i+1)*TI)  # 一维分块
    
    # 声明缓冲区
    L1_X = aries.buffer((TI,), "float32")
    L1_Y = aries.buffer((TI,), "float32")
    
    # 加载数据
    L1_X = aries.load(X, (ti,))
    
    for i in range(0, 32): 
        L1_Y[i] = L1_X[i] if L1_X[i] > 0.0 else 0.0
        
    # 存储结果
    aries.store(L1_Y, Y, (ti,))

# 顶层任务（一维向量处理）
@task_top()
def top_1d(X: float32[64], Y: float32[64]):
    N = 64
    TI = 32  # 分块大小
    grid = (N // TI, 1, 1)  # 一维网格
    tile_size = (TI, 1, 1)   # 一维分块
    relu_task = relu_tile_1d[grid, tile_size](X, Y)
    return relu_task

# ======== 项目配置和测试 ========
prj_dir = cur_dir + '/relu_1d_project'  # 修改项目目录名
temp_dir = aries_path + '/templates'
module = locals()

# 测试数据（一维向量）
N = 64
X = np.random.randn(N).astype(np.float32)  # 包含负值测试
Y = np.zeros(N, dtype=np.float32)

relu_task = top_1d(X, Y)
sch = Schedule(relu_task)

# 调度优化（针对一维特性）
sch.parallel(relu_task, [2, 1, 1])  # 只需在第一个维度并行
sch.l2buffer(relu_task, [2, 1, 1])   # L2缓存配置
sch.bufsel(relu_task, [1, 0, 0])     # 缓冲区选择
sch.redLoop(relu_task, [0])           # 无额外归约维度
sch.to("VCK190")                      # 目标平台

# 构建项目
sch.build(module, prj_dir, temp_dir)