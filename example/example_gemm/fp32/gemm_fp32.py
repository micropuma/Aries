import os
import sys
directory = os.path.dirname(os.path.abspath(__file__)) + "/../../../"
sys.path.append(directory)
from frontend import *

# ----------------------------
# Example usage
# ----------------------------
I, J, K= TypeVar("I"), TypeVar("J"), TypeVar("K")

@task_tile()
def gemm(A: float32[I, K], B: float32[K, J], C: float32[I, J]):

    i, j, k = tile_ranks()  # Tile ranks
    TI, TJ, TK = tile_sizes()  # Tile sizes

    ti = arange(i*TI, (i+1)*TI)  # I tile range
    tj = arange(j*TJ, (j+1)*TJ)  # J tile range
    tk = arange(k*TK, (k+1)*TK)  # K tile range

    A_L1: float32[TI, TK] = A[ti, tk]  # Load to L1 in core
    B_L1: float32[TK, TJ] = B[tk, tj]  # Load to L1 in core
    C_L1: float32[TI, TJ] = C[ti, tj]  # Load to L1 in core

    for i0 in range(0, TI):
        for j0 in range(0, TJ):
            for k0 in reduction_range(0, TK):
                C_L1[i0, j0] += A_L1[i0, k0] * B_L1[k0, j0]

    C[ti, tj] = C_L1  # PSUM


def app_gemm():
    I, J, K = 2816, 3072, 8192
    TI, TJ, TK = 32, 32, 32  # Tile sizes

    grid, size = (I/TI, J/TJ, K/TK), (TI, TJ, TK)
    A = np.random.rand(I, K)
    B = np.random.rand(K, J)
    C = np.zeros((I, J))
    task0 = gemm[grid, size](A, B, C)

    sch = Schedule(task0)
    code, _ = sch.build(target=VCK190)
    print(code)
    return


if __name__ == "__main__":
    app_gemm()