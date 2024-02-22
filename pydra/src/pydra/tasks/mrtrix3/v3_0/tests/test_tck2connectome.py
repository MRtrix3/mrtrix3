# Auto-generated test for tck2connectome

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import tck2connectome


def test_tck2connectome(tmp_path, cli_parse_only):

    task = tck2connectome(
        tracks_in=Tracks.sample(),
        nodes_in=Nifti1.sample(),
        connectome_out=File.sample(),
        assignment_end_voxels=True,
        assignment_radial_search=1.0,
        assignment_reverse_search=1.0,
        assignment_forward_search=1.0,
        assignment_all_voxels=True,
        scale_length=True,
        scale_invlength=True,
        scale_invnodevol=True,
        scale_file=Nifti1.sample(),
        symmetric=True,
        zero_diagonal=True,
        stat_edge="sum",
        tck_weights_in=File.sample(),
        keep_unassigned=True,
        out_assignments=False,
        vector=True,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
