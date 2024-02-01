from fileformats.core import hook
from fileformats.core.mixin import WithAdjacentFiles
from fileformats.medimage import DwiEncoding, Nifti1, NiftiGz, NiftiX, NiftiGzX
from .image import ImageFormat, ImageHeader, ImageFormatGz

class BFile(DwiEncoding):
    """MRtrix-style diffusion encoding, all in one file"""

    ext = ".b"


# NIfTI file format gzipped with BIDS side car
class WithBFile(WithAdjacentFiles):
    @hook.required
    @property
    def encoding(self) -> BFile:
        return BFile(self.select_by_ext(BFile))


class NiftiB(WithBFile, Nifti1):
    iana_mime = "application/x-nifti2+b"


class NiftiGzB(WithBFile, NiftiGz):
    iana_mime = "application/x-nifti2+gzip.b"


class NiftiXB(WithBFile, NiftiX):
    iana_mime = "application/x-nifti2+json.b"


class NiftiGzXB(WithBFile, NiftiGzX):
    iana_mime = "application/x-nifti2+gzip.json.b"


class ImageFormatB(WithBFile, ImageFormat):
    pass


class ImageFormatGzB(WithBFile, ImageFormatGz):
    pass


class ImageHeaderB(WithBFile, ImageHeader):
    pass
