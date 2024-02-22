# Auto-generated test for tcksample

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import tcksample


def test_tcksample(tmp_path, cli_parse_only):

    task = tcksample(
        tracks=Tracks.sample(),
        image_=Nifti1.sample(),
        values=File.sample(),
        stat_tck="mean",
        nointerp=True,
        precise=True,
        use_tdi_fraction=True,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
