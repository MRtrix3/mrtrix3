# Auto-generated test for dwibiascorrect_fsl

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import dwibiascorrect_fsl


def test_dwibiascorrect_fsl(tmp_path, cli_parse_only):
    task = dwibiascorrect_fsl(
        input=File.sample(),
        output=FsObject.sample(),
        grad=File.sample(),
        fslgrad=File.sample(),
        mask=File.sample(),
        bias=File.sample(),
        nocleanup=True,
        scratch=File.sample(),
        cont=File.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
