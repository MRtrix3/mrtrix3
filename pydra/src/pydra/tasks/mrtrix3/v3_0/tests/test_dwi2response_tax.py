# Auto-generated test for dwi2response_tax

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import dwi2response_tax


def test_dwi2response_tax(tmp_path, cli_parse_only):
    task = dwi2response_tax(
        input=File.sample(),
        output=FsObject.sample(),
        peak_ratio=1.0,
        max_iters=1,
        convergence=1.0,
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
