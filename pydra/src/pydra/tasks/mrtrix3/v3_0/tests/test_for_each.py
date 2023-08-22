# Auto-generated test for for_each

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import for_each


def test_for_each(tmp_path, cli_parse_only):
    task = for_each(
        inputs=File.sample(),
        colon=":",
        command="a-string",
        nocleanup=True,
        scratch=File.sample(),
        cont=File.sample(),
        debug=True,
        force=True,
        exclude=[File.sample()],
        test=True,
    )
    result = task(plugin="serial")
    assert not result.errored
