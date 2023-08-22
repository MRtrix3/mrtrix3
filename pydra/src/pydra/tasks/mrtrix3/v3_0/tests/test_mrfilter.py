# Auto-generated test for mrfilter

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import mrfilter


def test_mrfilter(tmp_path, cli_parse_only):
    task = mrfilter(
        input=Nifti1.sample(),
        filter="fft",
        output=ImageFormat.sample(),
        axes=list([1]),
        inverse=True,
        magnitude=True,
        rescale=True,
        centre_zero=True,
        stdev=list([1.0]),
        scanner=True,
        extent=list([1]),
        fwhm=list([1.0]),
        zupper=1.0,
        zlower=1.0,
        bridge=1,
        maskin=Nifti1.sample(),
        maskout=False,
        strides=File.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
