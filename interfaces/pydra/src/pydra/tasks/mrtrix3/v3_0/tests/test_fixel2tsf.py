# Auto-generated test for fixel2tsf

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import fixel2tsf


def test_fixel2tsf(tmp_path, cli_parse_only):

    task = fixel2tsf(
        fixel_in=Nifti1.sample(),
        tracks=Tracks.sample(),
        tsf=File.sample(),
        angle=1.0,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
