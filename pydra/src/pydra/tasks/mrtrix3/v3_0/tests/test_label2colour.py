# Auto-generated test for label2colour

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import label2colour


def test_label2colour(tmp_path, cli_parse_only):

    task = label2colour(
        nodes_in=Nifti1.sample(),
        colour_out=ImageFormat.sample(),
        lut=File.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
