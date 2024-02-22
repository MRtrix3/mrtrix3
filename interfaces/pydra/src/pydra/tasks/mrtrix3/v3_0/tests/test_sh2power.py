# Auto-generated test for sh2power

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import sh2power


def test_sh2power(tmp_path, cli_parse_only):

    task = sh2power(
        SH=Nifti1.sample(),
        power=ImageFormat.sample(),
        spectrum=True,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
