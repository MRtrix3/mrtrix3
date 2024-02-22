# Auto-generated test for dwidenoise

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import dwidenoise


def test_dwidenoise(tmp_path, cli_parse_only):

    task = dwidenoise(
        dwi=Nifti1.sample(),
        out=ImageFormat.sample(),
        mask=Nifti1.sample(),
        extent=list([1]),
        noise=False,
        rank=False,
        datatype="float32",
        estimator="exp1",
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
