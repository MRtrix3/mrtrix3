# Auto-generated test for tckresample

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import tckresample


def test_tckresample(tmp_path, cli_parse_only):

    task = tckresample(
        in_tracks=Tracks.sample(),
        out_tracks=Tracks.sample(),
        upsample=1,
        downsample=1,
        step_size=1.0,
        num_points=1,
        endpoints=True,
        line=tuple([1, 1, 1]),
        arc=tuple([1, 1, 1, 1]),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
