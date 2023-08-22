# Auto-generated test for dwinormalise_mtnorm

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import dwinormalise_mtnorm


def test_dwinormalise_mtnorm(tmp_path, cli_parse_only):
    task = dwinormalise_mtnorm(
        input=File.sample(),
        output=FsObject.sample(),
        grad=File.sample(),
        fslgrad=File.sample(),
        lmax=File.sample(),
        mask=File.sample(),
        reference=1.0,
        scale=File.sample(),
        nocleanup=True,
        scratch=File.sample(),
        cont=File.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
