# Auto-generated test for warpcorrect

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import warpcorrect


def test_warpcorrect(tmp_path, cli_parse_only):

    task = warpcorrect(
        in_=Nifti1.sample(),
        out=ImageFormat.sample(),
        marker=list([1.0]),
        tolerance=1.0,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
