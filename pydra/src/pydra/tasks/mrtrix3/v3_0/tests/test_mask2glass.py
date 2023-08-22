# Auto-generated test for mask2glass

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import mask2glass


def test_mask2glass(tmp_path, cli_parse_only):
    task = mask2glass(
        input=File.sample(),
        output=FsObject.sample(),
        nocleanup=True,
        scratch=File.sample(),
        cont=File.sample(),
        debug=True,
        force=True,
        dilate=1,
        scale=1.0,
        smooth=1.0,
    )
    result = task(plugin="serial")
    assert not result.errored
