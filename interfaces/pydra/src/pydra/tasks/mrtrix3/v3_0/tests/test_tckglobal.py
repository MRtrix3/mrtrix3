# Auto-generated test for tckglobal

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import tckglobal


def test_tckglobal(tmp_path, cli_parse_only):

    task = tckglobal(
        source=Nifti1.sample(),
        response=File.sample(),
        tracks=Tracks.sample(),
        grad=File.sample(),
        mask=Nifti1.sample(),
        riso=[File.sample()],
        lmax=1,
        length=1.0,
        weight=1.0,
        ppot=1.0,
        cpot=1.0,
        t0=1.0,
        t1=1.0,
        niter=1,
        fod=False,
        noapo=True,
        fiso=False,
        eext=False,
        etrend=False,
        balance=1.0,
        density=1.0,
        prob=list([1.0]),
        beta=1.0,
        lambda_=1.0,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
