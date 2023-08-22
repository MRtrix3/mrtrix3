# Auto-generated test for dwi2mask_3dautomask

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import dwi2mask_3dautomask


def test_dwi2mask_3dautomask(tmp_path, cli_parse_only):
    task = dwi2mask_3dautomask(
        input=File.sample(),
        output=FsObject.sample(),
        clfrac=1.0,
        nograd=True,
        peels=1.0,
        nbhrs=1,
        eclip=True,
        SI=1.0,
        dilate=1,
        erode=1,
        NN1=True,
        NN2=True,
        NN3=True,
        grad=File.sample(),
        fslgrad=File.sample(),
        nocleanup=True,
        scratch=File.sample(),
        cont=File.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
