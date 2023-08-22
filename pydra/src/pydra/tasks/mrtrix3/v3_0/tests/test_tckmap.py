# Auto-generated test for tckmap

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import tckmap


def test_tckmap(tmp_path, cli_parse_only):
    task = tckmap(
        tracks=File.sample(),
        output=ImageFormat.sample(),
        template=Nifti1.sample(),
        vox=list([1.0]),
        datatype="float16",
        dec=True,
        dixel=File.sample(),
        tod=1,
        contrast="tdi",
        image_=Nifti1.sample(),
        vector_file=File.sample(),
        stat_vox="sum",
        stat_tck="sum",
        fwhm_tck=1.0,
        map_zero=True,
        backtrack=True,
        upsample=1,
        precise=True,
        ends_only=True,
        tck_weights_in=File.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
