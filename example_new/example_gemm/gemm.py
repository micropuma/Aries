import os
import sys
cur_dir = os.path.dirname(os.path.abspath(__file__))
aries_path = cur_dir + "/../../"
sys.path.append(aries_path)
from newfrontend import *

@task_kernel(external_path="kernel_mm/aie_fp32_v0", para = [32,32,32])
def kernel_gemm(TileA: float32[32, 32], 
            TileB: float32[32, 32], 
            TileC: float32[32, 32]):
    for i0 in range(0, 32):
        for j0 in range(0, 32):
            for k0 in range(0, 32):
                TileC[i0, j0] += TileA[i0, k0] * TileB[k0, j0]

@task_tile(False) # Not run it on CPU
def gemm(A: float32[2816, 8192], B: float32[8192, 3072], C: float32[2816, 3072], **kwargs):
    i, j, k = aries.tile_ranks(**kwargs) # Grid indices (0, 0, 0) to (0, 1, 1)
    TI, TJ, TK = aries.tile_sizes(**kwargs)

    # Compute tile slices for multiple dimensions
    ti = aries.arange(i*TI, (i+1)*TI)  # I tile range
    tj = aries.arange(j*TJ, (j+1)*TJ)  # J tile range
    tk = aries.arange(k*TK, (k+1)*TK)  # K tile range
    
    L1_A = aries.buffer((TI, TK), "float32")
    L1_B = aries.buffer((TK, TJ), "float32")
    L1_C = aries.buffer((TI, TJ), "float32")
    
    L1_A = aries.load(A, (ti, tk))
    L1_B = aries.load(B, (tk, tj))
    L1_C = aries.load(C, (ti, tj))
    kernel_gemm(L1_A, L1_B, L1_C)
    aries.store(L1_C, C, (ti, tj))

@task_top()
def top(A: float32[2816, 8192], B: float32[8192, 3072], C: float32[2816, 3072]):
    I, J, K = 2816, 3072, 8192
    TI, TJ, TK = 32, 32, 32
    grid = (I // TI, J // TJ, K // TK)  # 2D grid
    tile_size = (TI, TJ, TK)  # 2D tile size
    gemm_task = gemm[grid, tile_size](A, B, C)
    return gemm_task
 
# Set the project dir and template dir
prj_dir= cur_dir + '/my_project'
temp_dir= aries_path + '/templates'
module = locals()
    
# Test with 2D array
I, J, K = 2816, 3072, 8192
A = np.random.rand(I, K).astype(np.float32)
B = np.random.rand(K, J).astype(np.float32)
C = np.random.rand(I, J).astype(np.float32)
gemm_task = top(A, B, C)
sch = Schedule(gemm_task)
sch.parallel(gemm_task, [1, 2, 2])
sch.l2buffer(gemm_task, [4, 6, 1])
sch.redLoop(gemm_task, [2])
sch.to("VCK190")
sch.build(module, prj_dir, temp_dir)