from fileformats.core import mark
from fileformats.core.mixin import WithAdjacentFiles
from fileformats.generic import File
from fileformats.medimage import DwiEncoding


class Bfile(DwiEncoding):
    """MRtrix-style diffusion encoding, all in one file"""

    ext = ".b"


# NIfTI file format gzipped with BIDS side car
class WithBfile(WithAdjacentFiles):
    @mark.required
    @property
    def encoding(self) -> Bfile:
        return Bfile(self.select_by_ext(Bfile))


class NiftiB(WithBfile, Nifti1):
    iana_mime = "application/x-nifti2+b"


class NiftiGzB(WithBfile, NiftiGz):
    iana_mime = "application/x-nifti2+gzip.b"


class NiftiXB(WithBfile, NiftiX):
    iana_mime = "application/x-nifti2+json.b"


class NiftiGzXB(WithBfile, NiftiGzX):
    iana_mime = "application/x-nifti2+gzip.json.b"


