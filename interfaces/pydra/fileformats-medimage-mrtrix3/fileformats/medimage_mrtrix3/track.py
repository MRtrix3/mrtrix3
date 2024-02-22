from fileformats.generic import File
from fileformats.core.mixin import WithMagicNumber


class Tracks(WithMagicNumber, File):

    ext = ".tck"
    magic_number = b"mrtrix tracks\n"
    binary = True
