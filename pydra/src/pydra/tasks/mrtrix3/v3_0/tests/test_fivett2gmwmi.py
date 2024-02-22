# Auto-generated test for fivett2gmwmi

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import fivett2gmwmi


def test_fivett2gmwmi(tmp_path, cli_parse_only):

    task = fivett2gmwmi(
        in_5tt=Nifti1.sample(),
        mask_out=ImageFormat.sample(),
        mask_in=Nifti1.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
