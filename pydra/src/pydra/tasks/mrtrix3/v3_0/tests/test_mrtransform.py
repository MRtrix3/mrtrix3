# Auto-generated test for mrtransform

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import mrtransform


def test_mrtransform(tmp_path, cli_parse_only):
    task = mrtransform(
        input=Nifti1.sample(),
        output=ImageFormat.sample(),
        linear=File.sample(),
        flip=list([1]),
        inverse=True,
        half=True,
        replace=File.sample(),
        identity=True,
        template=Nifti1.sample(),
        midway_space=True,
        interp="nearest",
        oversample=list([1]),
        warp=Nifti1.sample(),
        warp_full=Nifti1.sample(),
        from_=1,
        modulate="fod",
        directions=File.sample(),
        reorient_fod=True,
        grad=File.sample(),
        fslgrad=tuple([File.sample(), File.sample()]),
        export_grad_mrtrix=False,
        export_grad_fsl=False,
        datatype="float16",
        strides=File.sample(),
        nan=True,
        no_reorientation=True,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
