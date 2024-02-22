# Auto-generated test for tck2fixel

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import tck2fixel


def test_tck2fixel(tmp_path, cli_parse_only):

    task = tck2fixel(
        tracks=Tracks.sample(),
        fixel_folder_in=File.sample(),
        fixel_folder_out="a-string",
        fixel_data_out="a-string",
        angle=1.0,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
