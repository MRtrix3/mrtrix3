# Auto-generated test for dirgen

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import dirgen


def test_dirgen(tmp_path, cli_parse_only):

    task = dirgen(
        ndir=1,
        dirs=File.sample(),
        power=1,
        niter=1,
        restarts=1,
        unipolar=True,
        cartesian=True,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
