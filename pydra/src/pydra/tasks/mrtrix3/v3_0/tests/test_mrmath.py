# Auto-generated test for mrmath

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import mrmath


def test_mrmath(tmp_path, cli_parse_only):
    task = mrmath(
        input=[Nifti1.sample()],
        operation="mean",
        output=ImageFormat.sample(),
        axis=1,
        keep_unary_axes=True,
        datatype="float16",
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
