from ._version import __version__

from .image import ImageFormat, ImageHeader, ImageIn, ImageOut
from .dwi import (
    Bfile, NiftiB, NiftiGzB, NiftiGzXB, NiftiXB
)
from .track import Tracks
