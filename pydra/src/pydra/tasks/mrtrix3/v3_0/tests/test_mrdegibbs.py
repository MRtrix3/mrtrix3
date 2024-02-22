# Auto-generated test for mrdegibbs

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import mrdegibbs


def test_mrdegibbs(tmp_path, cli_parse_only):

    task = mrdegibbs(
        in_=Nifti1.sample(),
        out=ImageFormat.sample(),
        mode="2d",
        axes=list([1]),
        nshifts=1,
        minW=1,
        maxW=1,
        datatype="float16",
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
