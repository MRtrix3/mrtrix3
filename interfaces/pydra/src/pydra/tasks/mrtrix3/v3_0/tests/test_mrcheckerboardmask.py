# Auto-generated test for mrcheckerboardmask

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import mrcheckerboardmask


def test_mrcheckerboardmask(tmp_path, cli_parse_only):

    task = mrcheckerboardmask(
        input=Nifti1.sample(),
        output=ImageFormat.sample(),
        tiles=1,
        invert=True,
        nan=True,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
