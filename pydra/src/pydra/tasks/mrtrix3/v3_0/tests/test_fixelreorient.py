# Auto-generated test for fixelreorient

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import fixelreorient


def test_fixelreorient(tmp_path, cli_parse_only):

    task = fixelreorient(
        fixel_in=File.sample(),
        warp=Nifti1.sample(),
        fixel_out=Directory.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
