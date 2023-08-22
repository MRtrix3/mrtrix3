# Auto-generated test for tsfmult

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import tsfmult


def test_tsfmult(tmp_path, cli_parse_only):
    task = tsfmult(
        input1=File.sample(),
        output=File.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
