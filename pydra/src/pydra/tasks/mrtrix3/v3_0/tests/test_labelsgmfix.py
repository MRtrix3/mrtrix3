# Auto-generated test for labelsgmfix

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import labelsgmfix


def test_labelsgmfix(tmp_path, cli_parse_only):
    task = labelsgmfix(
        parc=File.sample(),
        t1=File.sample(),
        lut=File.sample(),
        output=FsObject.sample(),
        nocleanup=True,
        scratch=File.sample(),
        cont=File.sample(),
        debug=True,
        force=True,
        premasked=True,
        sgm_amyg_hipp=True,
    )
    result = task(plugin="serial")
    assert not result.errored
