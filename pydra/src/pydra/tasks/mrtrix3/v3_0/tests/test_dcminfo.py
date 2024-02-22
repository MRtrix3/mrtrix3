# Auto-generated test for dcminfo

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import dcminfo


def test_dcminfo(tmp_path, cli_parse_only):

    task = dcminfo(
        file=File.sample(),
        all=True,
        csa=True,
        phoenix=True,
        tag=[tuple(["a-string", "a-string"])],
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
