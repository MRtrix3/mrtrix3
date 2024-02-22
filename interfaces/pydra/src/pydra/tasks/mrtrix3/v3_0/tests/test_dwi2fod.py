# Auto-generated test for dwi2fod

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import dwi2fod


def test_dwi2fod(tmp_path, cli_parse_only):

    task = dwi2fod(
        algorithm="csd",
        dwi=Nifti1.sample(),
        response_odf=[File.sample()],
        grad=File.sample(),
        fslgrad=tuple([File.sample(), File.sample()]),
        shells=list([1.0]),
        directions=File.sample(),
        lmax=list([1]),
        mask=Nifti1.sample(),
        filter=File.sample(),
        neg_lambda=1.0,
        norm_lambda=1.0,
        threshold=1.0,
        niter=1,
        predicted_signal=False,
        strides=File.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
