# Auto-generated test for mrhistmatch

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import mrhistmatch


def test_mrhistmatch(tmp_path, cli_parse_only):
    task = mrhistmatch(
        type="scale",
        input=Nifti1.sample(),
        target=Nifti1.sample(),
        output=ImageFormat.sample(),
        mask_input=Nifti1.sample(),
        mask_target=Nifti1.sample(),
        bins=1,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
