# Auto-generated test for mrgrid

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import mrgrid


def test_mrgrid(tmp_path, cli_parse_only):

    task = mrgrid(
        input=Nifti1.sample(),
        operation="regrid",
        output=ImageFormat.sample(),
        template=Nifti1.sample(),
        size=list([1]),
        voxel=list([1.0]),
        scale=list([1.0]),
        interp="nearest",
        oversample=list([1]),
        as_=Nifti1.sample(),
        uniform=1,
        mask=Nifti1.sample(),
        crop_unbound=True,
        axis=[tuple([1, 1])],
        all_axes=True,
        fill=1.0,
        strides=File.sample(),
        datatype="float16",
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
