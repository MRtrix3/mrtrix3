# Auto-generated test for dirorder

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import dirorder


def test_dirorder(tmp_path, cli_parse_only):

    task = dirorder(
        input=File.sample(),
        output=File.sample(),
        cartesian=True,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
