# Auto-generated test for warp2metric

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import warp2metric


def test_warp2metric(tmp_path, cli_parse_only):

    task = warp2metric(
        in_=Nifti1.sample(),
        fc=tuple([Nifti1.sample(), Nifti1.sample(), Nifti1.sample()]),
        jmat=False,
        jdet=False,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
