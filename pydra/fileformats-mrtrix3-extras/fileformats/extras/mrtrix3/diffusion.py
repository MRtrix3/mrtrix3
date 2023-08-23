import numpy as np
from fileformats.medimage import DwiEncoding
from fileformats.mrtrix3 import BFile


@DwiEncoding.read_array.register
def bfile_read_array(bfile: BFile):
    return np.asarray(
        [[float(x) for x in ln.split()] for ln in bfile.read_contents().splitlines()]
    )
