# Auto-generated test for tsfsmooth

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import tsfsmooth


def test_tsfsmooth(tmp_path, cli_parse_only):

    task = tsfsmooth(
        input=File.sample(),
        output=File.sample(),
        stdev=1.0,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
