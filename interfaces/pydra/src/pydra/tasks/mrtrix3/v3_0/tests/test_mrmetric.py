# Auto-generated test for mrmetric

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import mrmetric


def test_mrmetric(tmp_path, cli_parse_only):

    task = mrmetric(
        image1=Nifti1.sample(),
        image2=Nifti1.sample(),
        space="voxel",
        interp="nearest",
        metric="diff",
        mask1=Nifti1.sample(),
        mask2=Nifti1.sample(),
        nonormalisation=True,
        overlap=True,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
