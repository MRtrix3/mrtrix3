# Auto-generated test for tcksift

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import tcksift


def test_tcksift(tmp_path, cli_parse_only):

    task = tcksift(
        in_tracks=Tracks.sample(),
        in_fod=Nifti1.sample(),
        out_tracks=Tracks.sample(),
        nofilter=True,
        output_at_counts=list([1]),
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
        out_selection=False,
        term_number=1,
        term_ratio=1.0,
        term_mu=1.0,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
