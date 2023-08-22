# Auto-generated test for dwi2mask_fslbet

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import dwi2mask_fslbet


def test_dwi2mask_fslbet(tmp_path, cli_parse_only):
    task = dwi2mask_fslbet(
        input=File.sample(),
        output=FsObject.sample(),
        bet_f=1.0,
        bet_g=1.0,
        bet_c=File.sample(),
        bet_r=1.0,
        rescale=True,
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
