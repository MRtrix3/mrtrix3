# Auto-generated test for labelconvert

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import labelconvert


def test_labelconvert(tmp_path, cli_parse_only):

    task = labelconvert(
        path_in=Nifti1.sample(),
        lut_in=File.sample(),
        lut_out=File.sample(),
        image_out=ImageFormat.sample(),
        spine=Nifti1.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
