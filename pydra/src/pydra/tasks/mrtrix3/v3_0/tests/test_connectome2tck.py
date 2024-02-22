# Auto-generated test for connectome2tck

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import connectome2tck


def test_connectome2tck(tmp_path, cli_parse_only):

    task = connectome2tck(
        tracks_in=File.sample(),
        assignments_in=File.sample(),
        prefix_out="a-string",
        nodes=list([1]),
        exclusive=True,
        files="per_edge",
        exemplars=Nifti1.sample(),
        keep_unassigned=True,
        keep_self=True,
        tck_weights_in=File.sample(),
        prefix_tck_weights_out="a-string",
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
