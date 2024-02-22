# Auto-generated test for fod2fixel

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import fod2fixel


def test_fod2fixel(tmp_path, cli_parse_only):

    task = fod2fixel(
        fod=Nifti1.sample(),
        fixel_directory=Directory.sample(),
        afd=False,
        peak_amp=False,
        disp=False,
        fmls_integral=1.0,
        fmls_peak_value=1.0,
        fmls_no_thresholds=True,
        fmls_lobe_merge_ratio=1.0,
        mask=Nifti1.sample(),
        maxnum=1,
        nii=True,
        dirpeak=True,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
