# Auto-generated test for connectomestats

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import connectomestats


def test_connectomestats(tmp_path, cli_parse_only):
    task = connectomestats(
        input=File.sample(),
        algorithm="nbs",
        design=File.sample(),
        contrast=File.sample(),
        output="a-string",
        notest=True,
        errors="ee",
        exchange_within=File.sample(),
        exchange_whole=File.sample(),
        strong=True,
        nshuffles=1,
        permutations=File.sample(),
        nonstationarity=True,
        skew_nonstationarity=1.0,
        nshuffles_nonstationarity=1,
        permutations_nonstationarity=File.sample(),
        tfce_dh=1.0,
        tfce_e=1.0,
        tfce_h=1.0,
        variance=File.sample(),
        ftests=File.sample(),
        fonly=True,
        column=[File.sample()],
        threshold=1.0,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
