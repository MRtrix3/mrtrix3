# Auto-generated test for fixelfilter

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import fixelfilter


def test_fixelfilter(tmp_path, cli_parse_only):

    task = fixelfilter(
        input=File.sample(),
        filter="connect",
        output=File.sample(),
        matrix=File.sample(),
        threshold_value=1.0,
        threshold_connectivity=1.0,
        fwhm=1.0,
        minweight=1.0,
        mask=Nifti1.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
