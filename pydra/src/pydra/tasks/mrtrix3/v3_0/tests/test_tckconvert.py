# Auto-generated test for tckconvert

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import tckconvert


def test_tckconvert(tmp_path, cli_parse_only):
    task = tckconvert(
        input=File.sample(),
        output=File.sample(),
        scanner2voxel=Nifti1.sample(),
        scanner2image=Nifti1.sample(),
        voxel2scanner=Nifti1.sample(),
        image2scanner=Nifti1.sample(),
        sides=1,
        increment=1,
        dec=True,
        radius=1.0,
        ascii=True,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
