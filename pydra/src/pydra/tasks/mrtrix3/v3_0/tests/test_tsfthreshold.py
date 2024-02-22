# Auto-generated test for tsfthreshold

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import tsfthreshold


def test_tsfthreshold(tmp_path, cli_parse_only):

    task = tsfthreshold(
        input=File.sample(),
        T=1.0,
        output=File.sample(),
        invert=True,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
