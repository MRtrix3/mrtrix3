# Auto-generated test for tcksift2

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import tcksift2


def test_tcksift2(tmp_path, cli_parse_only):

    task = tcksift2(
        in_tracks=Tracks.sample(),
        in_fod=Nifti1.sample(),
        out_weights=File.sample(),
        proc_mask=Nifti1.sample(),
        act=Nifti1.sample(),
        fd_scale_gm=True,
        no_dilate_lut=True,
        make_null_lobes=True,
        remove_untracked=True,
        fd_thresh=1.0,
        csv=False,
        out_mu=False,
        output_debug=False,
        out_coeffs=False,
        reg_tikhonov=1.0,
        reg_tv=1.0,
        min_td_frac=1.0,
        min_iters=1,
        max_iters=1,
        min_factor=1.0,
        min_coeff=1.0,
        max_factor=1.0,
        max_coeff=1.0,
        max_coeff_step=1.0,
        min_cf_decrease=1.0,
        linear=True,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
