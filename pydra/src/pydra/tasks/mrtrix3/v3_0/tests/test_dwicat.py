# Auto-generated test for dwicat

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import dwicat


def test_dwicat(tmp_path, cli_parse_only):
    task = dwicat(
        inputs=File.sample(),
        output=FsObject.sample(),
        nocleanup=True,
        scratch=File.sample(),
        cont=File.sample(),
        debug=True,
        force=True,
        mask=File.sample(),
    )
    result = task(plugin="serial")
    assert not result.errored
