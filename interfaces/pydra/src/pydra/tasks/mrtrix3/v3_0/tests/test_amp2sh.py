# Auto-generated test for amp2sh

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import amp2sh


def test_amp2sh(tmp_path, cli_parse_only):

    task = amp2sh(
        amp=Nifti1.sample(),
        SH=ImageFormat.sample(),
        lmax=1,
        normalise=True,
        directions=File.sample(),
        rician=Nifti1.sample(),
        grad=File.sample(),
        fslgrad=tuple([File.sample(), File.sample()]),
        shells=list([1.0]),
        strides=File.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
