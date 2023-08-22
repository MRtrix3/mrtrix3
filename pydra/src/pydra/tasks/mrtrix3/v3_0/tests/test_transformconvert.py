# Auto-generated test for transformconvert

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import transformconvert


def test_transformconvert(tmp_path, cli_parse_only):
    task = transformconvert(
        input=[File.sample()],
        operation="flirt_import",
        output=File.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
