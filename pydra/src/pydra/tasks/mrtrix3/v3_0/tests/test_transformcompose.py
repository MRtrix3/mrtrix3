# Auto-generated test for transformcompose

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import transformcompose


def test_transformcompose(tmp_path, cli_parse_only):

    task = transformcompose(
        input=[File.sample()],
        output=File.sample(),
        template=Nifti1.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
