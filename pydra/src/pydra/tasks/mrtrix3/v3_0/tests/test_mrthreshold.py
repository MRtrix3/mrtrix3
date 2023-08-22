# Auto-generated test for mrthreshold

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import mrthreshold


def test_mrthreshold(tmp_path, cli_parse_only):
    task = mrthreshold(
        input=Nifti1.sample(),
        output=ImageFormat.sample(),
        abs=1.0,
        percentile=1.0,
        top=1,
        bottom=1,
        allvolumes=True,
        ignorezero=True,
        mask=Nifti1.sample(),
        comparison="lt",
        invert=True,
        out_masked=True,
        nan=True,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
