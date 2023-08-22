# Auto-generated test for mrtrix_cleanup

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import mrtrix_cleanup


def test_mrtrix_cleanup(tmp_path, cli_parse_only):
    task = mrtrix_cleanup(
        path=File.sample(),
        nocleanup=True,
        scratch=File.sample(),
        cont=File.sample(),
        debug=True,
        force=True,
        test=True,
        failed=File.sample(),
    )
    result = task(plugin="serial")
    assert not result.errored
