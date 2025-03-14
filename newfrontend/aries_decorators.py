import itertools
from typing import Any, Callable, Tuple

# ================== Decorators ===================
class TaskInstance:
    def __init__(self, func, grid_dims, tile_sizes, instanceIdx, top_flag, call_args, call_kwargs):
        self.func = func
        self.grid_dims = grid_dims
        self.tile_sizes = tile_sizes
        self.instanceIdx = instanceIdx
        self.top_flag = top_flag
        self.call_args = call_args
        self.call_kwargs = call_kwargs

class TaskTileWrapper:
    def __init__(self, func: Callable, run=True):
        self.func = func
        self.grid_dims = None
        self.tile_sizes = None
        self.instanceIdx = 0
        self.run = run

    # 通过[]语法定义dim size
    # 调用如下：
    # ttmc_task = ttmc[grid, size](A, B, C, D) 
    # grid为[]的args[0], size为[]的args[1]
    def __getitem__(self, args: Tuple[Any, ...]):
        assert len(args) >=2
        self.grid_dims = args[0]  # Grid dimensions (e.g., (4, 4) for 2D tiling)
        self.tile_sizes = args[1]  # Tile sizes (e.g., (32, 32))
        if len(args) == 3:
            self.instanceIdx = args[2] # Mark the number of instance
        return self

    # *call_args是元组，**call_kwargs是字典
    def __call__(self, *call_args, **call_kwargs):
        if not self.grid_dims or not self.tile_sizes:
            raise ValueError(
                "Grid dimensions and tile sizes must be specified")
        
        # 单个维度的grid_dims，转换为tuple
        if isinstance(self.grid_dims, int):
            self.grid_dims = (self.grid_dims,)
        
        # Generate all tile index combinations (e.g., (i, j) for a 2D grid)
        tile_indices = itertools.product(*(range(g) for g in self.grid_dims))
        call_kwargs['TSizes'] = self.tile_sizes  # Add tuple of sizes
        if self.run:
            for idx in tile_indices:
                call_kwargs['IVs'] = idx  # Add tuple of indices
                self.func(*call_args, **call_kwargs)

        # 解析出参数，并生成TaskInstance
        instance = TaskInstance(self.func, self.grid_dims, self.tile_sizes, 
                                self.instanceIdx, False, call_args, call_kwargs)
        return instance

# 如下使用：
# @task_tile(False) # Not run it on CPU
# def ttmc(A: float32[4, 1024, 4096], B: float32[1024, 512], 
#          C: float32[4096, 768],    D: float32[4, 512, 768], **kwargs):
# 上述是函数原型
# ttmc_task = ttmc[grid, size](A, B, C, D)
# 上述是调用
def task_tile(run_flag=True):  
    """Decorator to wrap the function for tiles."""
    def decorator(func: Callable):
        # TaskTileWrapper根据用户的配置，生成多个TaskInstance
        return TaskTileWrapper(func, run_flag)
    return decorator

class TaskKernelWrapper:
    def __init__(self, func: Callable, external_kernel = None, para = []):
        self.func = func
        self.externKrnl = external_kernel
        self.para = para
    def __call__(self, *call_args, **call_kwargs):
        self.func(*call_args, **call_kwargs)

def task_kernel(*, external_path=None, para = []):  
    """Decorator to wrap the function for single AIE."""
    def decorator(func: Callable):
        return TaskKernelWrapper(func, external_path, para)
    return decorator

class TaskTopWrapper:
    def __init__(self, func: Callable):
        self.func = func
    def __call__(self, *call_args, **call_kwargs):
        return self.func(*call_args, **call_kwargs)

def task_top():  
    """Decorator to wrap the function for top function."""
    def decorator(func: Callable):
        return TaskTopWrapper(func)
    return decorator
  
# ======= Reduction range ========
class ReductionRange:
    def __init__(self, start, stop, step=1):
        if stop is None:
            start, stop = 0, start
        self.range = range(start, stop, step)
        
    def __iter__(self):
        return iter(self.range)

# Helper function to create a ReductionRange
def reduction_range(start, stop, step=1):
    return ReductionRange(start, stop, step)