# Auto-generated test for warpconvert

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import warpconvert


def test_warpconvert(tmp_path, cli_parse_only):

    task = warpconvert(
        in_=Nifti1.sample(),
        type="deformation2displacement",
        out=ImageFormat.sample(),
        template=Nifti1.sample(),
        midway_space=True,
        from_=1,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
