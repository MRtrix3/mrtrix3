# Auto-generated test for dwi2mask_trace

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import dwi2mask_trace


def test_dwi2mask_trace(tmp_path, cli_parse_only):
    task = dwi2mask_trace(
        input=File.sample(),
        output=FsObject.sample(),
        iterative=True,
        max_iters=1,
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
