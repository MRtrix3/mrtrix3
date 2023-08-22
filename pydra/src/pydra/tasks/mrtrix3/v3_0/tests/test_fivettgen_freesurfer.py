# Auto-generated test for fivettgen_freesurfer

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import fivettgen_freesurfer


def test_fivettgen_freesurfer(tmp_path, cli_parse_only):
    task = fivettgen_freesurfer(
        input=File.sample(),
        output=FsObject.sample(),
        lut=File.sample(),
        nocrop=True,
        sgm_amyg_hipp=True,
        nocleanup=True,
        scratch=File.sample(),
        cont=File.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
