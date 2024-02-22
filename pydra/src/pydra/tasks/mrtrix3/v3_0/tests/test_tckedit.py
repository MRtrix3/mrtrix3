# Auto-generated test for tckedit

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import tckedit


def test_tckedit(tmp_path, cli_parse_only):

    task = tckedit(
        tracks_in=[Tracks.sample()],
        tracks_out=Tracks.sample(),
        include=[File.sample()],
        include_ordered=["a-string"],
        exclude=[File.sample()],
        mask=[File.sample()],
        maxlength=1.0,
        minlength=1.0,
        number=1,
        skip=1,
        maxweight=1.0,
        minweight=1.0,
        inverse=True,
        ends_only=True,
        tck_weights_in=File.sample(),
        tck_weights_out=False,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
