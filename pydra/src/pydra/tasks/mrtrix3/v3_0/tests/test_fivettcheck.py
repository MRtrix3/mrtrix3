# Auto-generated test for fivettcheck

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import fivettcheck


def test_fivettcheck(tmp_path, cli_parse_only):

    task = fivettcheck(
        input=[Nifti1.sample()],
        voxels="a-string",
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
