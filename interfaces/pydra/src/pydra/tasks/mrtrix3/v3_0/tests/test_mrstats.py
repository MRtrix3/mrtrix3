# Auto-generated test for mrstats

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import mrstats


def test_mrstats(tmp_path, cli_parse_only):

    task = mrstats(
        image_=Nifti1.sample(),
        output="mean",
        mask=Nifti1.sample(),
        ignorezero=True,
        allvolumes=True,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
