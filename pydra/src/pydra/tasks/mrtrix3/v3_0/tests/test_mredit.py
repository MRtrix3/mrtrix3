# Auto-generated test for mredit

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import mredit


def test_mredit(tmp_path, cli_parse_only):
    task = mredit(
        input=Nifti1.sample(),
        output=ImageFormat.sample(),
        plane=[tuple([1, 1, 1])],
        sphere=[tuple([list([1.0]), list([1.0]), list([1.0])])],
        voxel=[tuple([list([1.0]), list([1.0])])],
        scanner=True,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
