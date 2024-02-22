# Auto-generated test for tensor2metric

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import tensor2metric


def test_tensor2metric(tmp_path, cli_parse_only):

    task = tensor2metric(
        tensor=Nifti1.sample(),
        mask=Nifti1.sample(),
        adc=False,
        fa=False,
        ad=False,
        rd=False,
        value=False,
        vector=False,
        num=list([1]),
        modulate="none",
        cl=False,
        cp=False,
        cs=False,
        dkt=Nifti1.sample(),
        mk=False,
        ak=False,
        rk=False,
        mk_dirs=File.sample(),
        rk_ndirs=1,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
