# Auto-generated test for tcktransform

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import tcktransform


def test_tcktransform(tmp_path, cli_parse_only):

    task = tcktransform(
        tracks=Tracks.sample(),
        transform=Nifti1.sample(),
        output=Tracks.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
