# Auto-generated test for vectorstats

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import vectorstats


def test_vectorstats(tmp_path, cli_parse_only):
    task = vectorstats(
        input=File.sample(),
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
        variance=File.sample(),
        ftests=File.sample(),
        fonly=True,
        column=[File.sample()],
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
