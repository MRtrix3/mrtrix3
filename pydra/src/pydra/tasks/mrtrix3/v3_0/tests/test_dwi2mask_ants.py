# Auto-generated test for dwi2mask_ants

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import dwi2mask_ants


def test_dwi2mask_ants(tmp_path, cli_parse_only):
    task = dwi2mask_ants(
        input=File.sample(),
        output=FsObject.sample(),
        template=File.sample(),
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
