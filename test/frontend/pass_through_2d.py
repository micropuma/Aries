import os
import sys
directory = os.path.dirname(os.path.abspath(__file__)) + "/../../"
sys.path.append(directory)
from newfrontend import *

@task_kernel()
def compute(chunk_in: float32[32, 32], chunk_out: float32[32, 32]):
    for i0 in range(0, 32):
        for j0 in range(0, 32):
            chunk_out[i0, j0] = chunk_in[i0, j0] * float32(1) + float32(1) + float32(11)

@task_tile()
def pass_through(A: float32[64, 64], C: float32[64, 64], **kwargs):
    i, j = aries.tile_ranks(**kwargs)
    TI, TJ = aries.tile_sizes(**kwargs)

    # Compute tile slices for multiple dimensions
    ti = aries.arange(i*TI, (i+1)*TI)  # I tile range
    tj = aries.arange(j*TJ, (j+1)*TJ)  # J tile range
    
    chunk_in = aries.buffer((TI, TJ), "float32")
    chunk_out = aries.buffer((TI, TJ), "float32")
    
    chunk_in = aries.load(A, (ti, tj))
    compute(chunk_in, chunk_out)
    aries.store(chunk_out, C, (ti, tj))

@task_top()
def top(A: float32[64, 64], B: float32[64, 64]):
    I, J = 64, 64
    TI, TJ = 32, 32
    grid = (I // TI, J // TJ)  # 2D grid
    tile_size = (TI, TJ)  # 2D tile size
    instance = pass_through[grid, tile_size](A, B)
    return instance
    
# Test with 2D array
I, J = 64, 64
A = np.random.rand(I, J).astype(np.float32)
B = np.zeros((I, J)).astype(np.float32)
instance = top(A, B)
sch = Schedule(instance)
sch.build(locals())
#print(B)