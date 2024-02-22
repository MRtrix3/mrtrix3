# Auto-generated test for fod2dec

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import fod2dec


def test_fod2dec(tmp_path, cli_parse_only):

    task = fod2dec(
        input=Nifti1.sample(),
        output=ImageFormat.sample(),
        mask=Nifti1.sample(),
        contrast=Nifti1.sample(),
        lum=True,
        lum_coefs=list([1.0]),
        lum_gamma=1.0,
        threshold=1.0,
        no_weight=True,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
