# Auto-generated test for meshconvert

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import meshconvert


def test_meshconvert(tmp_path, cli_parse_only):
    task = meshconvert(
        input=File.sample(),
        output=File.sample(),
        binary=True,
        transform=tuple(["a-string", "a-string"]),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
