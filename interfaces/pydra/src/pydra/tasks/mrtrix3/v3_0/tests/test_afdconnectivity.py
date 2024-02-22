# Auto-generated test for afdconnectivity

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import afdconnectivity


def test_afdconnectivity(tmp_path, cli_parse_only):

    task = afdconnectivity(
        image_=Nifti1.sample(),
        tracks=Tracks.sample(),
        wbft=Tracks.sample(),
        afd_map=False,
        all_fixels=True,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
