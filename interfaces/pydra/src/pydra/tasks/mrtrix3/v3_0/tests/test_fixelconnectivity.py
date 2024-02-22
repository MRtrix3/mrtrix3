# Auto-generated test for fixelconnectivity

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import fixelconnectivity


def test_fixelconnectivity(tmp_path, cli_parse_only):

    task = fixelconnectivity(
        fixel_directory=File.sample(),
        tracks=Tracks.sample(),
        matrix=Directory.sample(),
        threshold=1.0,
        angle=1.0,
        mask=Nifti1.sample(),
        tck_weights_in=File.sample(),
        count=False,
        extent=False,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
