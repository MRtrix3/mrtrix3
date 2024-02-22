# Auto-generated test for peaks2fixel

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import peaks2fixel


def test_peaks2fixel(tmp_path, cli_parse_only):

    task = peaks2fixel(
        directions=Nifti1.sample(),
        fixels=Directory.sample(),
        dataname="a-string",
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
