# Auto-generated test for fixel2sh

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import fixel2sh


def test_fixel2sh(tmp_path, cli_parse_only):

    task = fixel2sh(
        fixel_in=Nifti1.sample(),
        sh_out=ImageFormat.sample(),
        lmax=1,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
