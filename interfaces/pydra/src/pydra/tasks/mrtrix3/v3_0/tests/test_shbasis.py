# Auto-generated test for shbasis

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import shbasis


def test_shbasis(tmp_path, cli_parse_only):

    task = shbasis(
        SH=[Nifti1.sample()],
        convert="old",
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
