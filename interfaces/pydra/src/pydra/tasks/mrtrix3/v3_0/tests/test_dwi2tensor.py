# Auto-generated test for dwi2tensor

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import dwi2tensor


def test_dwi2tensor(tmp_path, cli_parse_only):

    task = dwi2tensor(
        dwi=Nifti1.sample(),
        dt=ImageFormat.sample(),
        ols=True,
        iter=1,
        constrain=True,
        directions=File.sample(),
        mask=Nifti1.sample(),
        b0=False,
        dkt=False,
        predicted_signal=False,
        grad=File.sample(),
        fslgrad=tuple([File.sample(), File.sample()]),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
