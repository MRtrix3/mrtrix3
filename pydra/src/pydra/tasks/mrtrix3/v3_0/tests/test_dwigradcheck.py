# Auto-generated test for dwigradcheck

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import dwigradcheck


def test_dwigradcheck(tmp_path, cli_parse_only):
    task = dwigradcheck(
        input=File.sample(),
        grad=File.sample(),
        fslgrad=File.sample(),
        export_grad_mrtrix=File.sample(),
        export_grad_fsl=File.sample(),
        nocleanup=True,
        scratch=File.sample(),
        cont=File.sample(),
        debug=True,
        force=True,
        mask=File.sample(),
        number=1,
    )
    result = task(plugin="serial")
    assert not result.errored
