# Auto-generated test for dcmedit

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import dcmedit


def test_dcmedit(tmp_path, cli_parse_only):

    task = dcmedit(
        file=File.sample(),
        anonymise=True,
        id="a-string",
        tag=[tuple([File.sample(), File.sample(), File.sample()])],
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
