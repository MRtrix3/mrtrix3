# Auto-generated test for responsemean

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import responsemean


def test_responsemean(tmp_path, cli_parse_only):
    task = responsemean(
        inputs=File.sample(),
        output=FsObject.sample(),
        nocleanup=True,
        scratch=File.sample(),
        cont=File.sample(),
        debug=True,
        force=True,
        legacy=True,
    )
    result = task(plugin="serial")
    assert not result.errored
