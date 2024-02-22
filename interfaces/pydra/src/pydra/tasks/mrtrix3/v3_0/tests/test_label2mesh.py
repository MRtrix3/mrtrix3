# Auto-generated test for label2mesh

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import label2mesh


def test_label2mesh(tmp_path, cli_parse_only):

    task = label2mesh(
        nodes_in=Nifti1.sample(),
        mesh_out=File.sample(),
        blocky=True,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
