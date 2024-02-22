# Auto-generated test for sh2amp

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import sh2amp


def test_sh2amp(tmp_path, cli_parse_only):

    task = sh2amp(
        input=Nifti1.sample(),
        directions=File.sample(),
        output=ImageFormat.sample(),
        nonnegative=True,
        grad=File.sample(),
        fslgrad=tuple([File.sample(), File.sample()]),
        strides=File.sample(),
        datatype="float16",
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
