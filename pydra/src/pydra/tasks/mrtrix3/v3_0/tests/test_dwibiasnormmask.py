# Auto-generated test for dwibiasnormmask

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import dwibiasnormmask


def test_dwibiasnormmask(tmp_path, cli_parse_only):
    task = dwibiasnormmask(
        input=File.sample(),
        output_dwi=File.sample(),
        output_mask=File.sample(),
        grad=File.sample(),
        fslgrad=File.sample(),
        dice=1.0,
        init_mask=File.sample(),
        max_iters=1,
        mask_algo="dwi2mask",
        lmax=File.sample(),
        output_bias=File.sample(),
        output_scale=File.sample(),
        output_tissuesum=File.sample(),
        reference=1.0,
        nocleanup=True,
        scratch=File.sample(),
        cont=File.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
