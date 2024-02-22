# Auto-generated test for mtnormalise

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import mtnormalise


def test_mtnormalise(tmp_path, cli_parse_only):

    task = mtnormalise(
        input_output=[File.sample()],
        mask=Nifti1.sample(),
        order="0",
        niter=list([1]),
        reference=1.0,
        balanced=True,
        check_norm=False,
        check_mask=False,
        check_factors=False,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
