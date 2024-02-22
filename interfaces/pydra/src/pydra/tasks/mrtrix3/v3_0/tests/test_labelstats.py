# Auto-generated test for labelstats

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import labelstats


def test_labelstats(tmp_path, cli_parse_only):

    task = labelstats(
        input=Nifti1.sample(),
        output="mass",
        voxelspace=True,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
