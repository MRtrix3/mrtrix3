# Auto-generated test for dwi2response_manual

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import dwi2response_manual


def test_dwi2response_manual(tmp_path, cli_parse_only):
    task = dwi2response_manual(
        input=File.sample(),
        in_voxels=File.sample(),
        output=FsObject.sample(),
        dirs=File.sample(),
        grad=File.sample(),
        fslgrad=File.sample(),
        mask=File.sample(),
        voxels=File.sample(),
        shells=File.sample(),
        lmax=File.sample(),
        nocleanup=True,
        scratch=File.sample(),
        cont=File.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
