# Auto-generated test for fixelcfestats

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import fixelcfestats


def test_fixelcfestats(tmp_path, cli_parse_only):

    task = fixelcfestats(
        in_fixel_directory=File.sample(),
        subjects=Nifti1.sample(),
        design=File.sample(),
        contrast=File.sample(),
        connectivity=File.sample(),
        out_fixel_directory="a-string",
        mask=Nifti1.sample(),
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
        cfe_dh=1.0,
        cfe_e=1.0,
        cfe_h=1.0,
        cfe_c=1.0,
        cfe_legacy=True,
        variance=File.sample(),
        ftests=File.sample(),
        fonly=True,
        column=[File.sample()],
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
