# Auto-generated test for tsfdivide

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import tsfdivide


def test_tsfdivide(tmp_path, cli_parse_only):
    task = tsfdivide(
        input1=File.sample(),
        input2=File.sample(),
        output=File.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
