# Auto-generated test for fixelcorrespondence

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import fixelcorrespondence


def test_fixelcorrespondence(tmp_path, cli_parse_only):

    task = fixelcorrespondence(
        subject_data=Nifti1.sample(),
        template_directory=File.sample(),
        output_directory="a-string",
        output_data="a-string",
        angle=1.0,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
