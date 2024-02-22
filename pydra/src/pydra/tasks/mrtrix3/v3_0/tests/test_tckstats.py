# Auto-generated test for tckstats

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import tckstats


def test_tckstats(tmp_path, cli_parse_only):

    task = tckstats(
        tracks_in=Tracks.sample(),
        output="mean",
        histogram=False,
        dump=False,
        ignorezero=True,
        tck_weights_in=File.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
