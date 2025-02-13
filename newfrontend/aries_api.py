from .aries_types import *

# =================== APIs ====================
class aries:
    @staticmethod
    def arange(start, stop, step=1):
        return slice(start, stop, step)

    @staticmethod
    def buffer(shape: Tuple[int, ...], dtype: str) -> Tensor:
        return Tensor(shape, dtype)
    
    @staticmethod
    def load(array, slices):
        return array[slices]
    
    @staticmethod
    def store(data, array, slices):
        array[slices] = data

    @staticmethod
    def tile_ranks(**kwargs):
        return kwargs.get('IVs')

    @staticmethod
    def tile_sizes(**kwargs):
        return kwargs.get('TSizes')