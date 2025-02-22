import os
import sys
directory = os.path.dirname(os.path.abspath(__file__)) + "/../../"
sys.path.append(directory)
from newfrontend import *

@task_kernel()
def compute(chunk_in: float32[32], chunk_out: float32[32]):
    for i0 in range(0, 32):
        chunk_out[i0] = chunk_in[i0] * float32(1) + float32(1)

@task_tile()
def pass_through(A: float32[64], C: float32[64], **kwargs):
    i = aries.tile_ranks(**kwargs)
    TI = aries.tile_sizes(**kwargs)
    # Compute tile slices for multiple dimensions
    ti = aries.arange(i*TI, (i+1)*TI)  # I tile range
    
    chunk_in = aries.buffer((TI,), "float32")
    chunk_out = aries.buffer((TI,), "float32")
    
    chunk_in = aries.load(A, ti)
    compute(chunk_in, chunk_out)
    aries.store(chunk_out, C, ti)

@task_top()
def top(A: float32[64], B: float32[64]):
    I = 64
    TI = 32
    grid = I // TI  # 1D grid
    tile_size = TI  # 1D tile size
    instance = pass_through[grid, tile_size](A, B)
    pass_through[grid, tile_size](A, B)
    return instance
    
# Test with 2D array
I = 64
A = np.random.rand(I).astype(np.float32)
B = np.zeros(I).astype(np.float32)
instance = top(A, B)
sch = Schedule(instance)
sch.build(locals())
# print(B)