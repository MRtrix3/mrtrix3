# Auto-generated test for dwinormalise_group

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import dwinormalise_group


def test_dwinormalise_group(tmp_path, cli_parse_only):
    task = dwinormalise_group(
        input_dir=File.sample(),
        mask_dir=File.sample(),
        output_dir=File.sample(),
        fa_template=File.sample(),
        wm_mask=File.sample(),
        nocleanup=True,
        scratch=File.sample(),
        cont=File.sample(),
        debug=True,
        force=True,
        fa_threshold=File.sample(),
    )
    result = task(plugin="serial")
    assert not result.errored
