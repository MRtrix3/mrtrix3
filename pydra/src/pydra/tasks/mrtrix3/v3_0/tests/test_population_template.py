# Auto-generated test for population_template

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import population_template


def test_population_template(tmp_path, cli_parse_only):
    task = population_template(
        input_dir=File.sample(),
        template=File.sample(),
        type=File.sample(),
        voxel_size=File.sample(),
        initial_alignment=File.sample(),
        mask_dir=File.sample(),
        warp_dir=File.sample(),
        transformed_dir=File.sample(),
        linear_transformations_dir=File.sample(),
        template_mask=File.sample(),
        noreorientation=True,
        leave_one_out=File.sample(),
        aggregate=File.sample(),
        aggregation_weights=File.sample(),
        nanmask=True,
        copy_input=True,
        delete_temporary_files=True,
        nl_scale=File.sample(),
        nl_lmax=File.sample(),
        nl_niter=File.sample(),
        nl_update_smooth=File.sample(),
        nl_disp_smooth=File.sample(),
        nl_grad_step=File.sample(),
        linear_no_pause=True,
        linear_no_drift_correction=True,
        linear_estimator=File.sample(),
        rigid_scale=File.sample(),
        rigid_lmax=File.sample(),
        rigid_niter=File.sample(),
        affine_scale=File.sample(),
        affine_lmax=File.sample(),
        affine_niter=File.sample(),
        mc_weight_initial_alignment=File.sample(),
        mc_weight_rigid=File.sample(),
        mc_weight_affine=File.sample(),
        mc_weight_nl=File.sample(),
        nocleanup=True,
        scratch=File.sample(),
        cont=File.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
