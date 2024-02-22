# Auto-generated test for tsfvalidate

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import tsfvalidate


def test_tsfvalidate(tmp_path, cli_parse_only):

    task = tsfvalidate(
        tsf=File.sample(),
        tracks=File.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
