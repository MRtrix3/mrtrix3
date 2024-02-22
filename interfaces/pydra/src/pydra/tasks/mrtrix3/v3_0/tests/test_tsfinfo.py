# Auto-generated test for tsfinfo

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import tsfinfo


def test_tsfinfo(tmp_path, cli_parse_only):

    task = tsfinfo(
        tracks=[File.sample()],
        count=True,
        ascii="a-string",
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
