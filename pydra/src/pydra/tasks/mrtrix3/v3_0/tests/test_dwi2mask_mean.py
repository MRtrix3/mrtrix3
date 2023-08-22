# Auto-generated test for dwi2mask_mean

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import dwi2mask_mean


def test_dwi2mask_mean(tmp_path, cli_parse_only):
    task = dwi2mask_mean(
        input=File.sample(),
        output=FsObject.sample(),
        shells=File.sample(),
        clean_scale=1,
        grad=File.sample(),
        fslgrad=File.sample(),
        nocleanup=True,
        scratch=File.sample(),
        cont=File.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
