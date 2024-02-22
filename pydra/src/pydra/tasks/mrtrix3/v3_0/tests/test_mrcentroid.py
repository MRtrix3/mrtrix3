# Auto-generated test for mrcentroid

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import mrcentroid


def test_mrcentroid(tmp_path, cli_parse_only):

    task = mrcentroid(
        input=Nifti1.sample(),
        mask=Nifti1.sample(),
        voxelspace=True,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
