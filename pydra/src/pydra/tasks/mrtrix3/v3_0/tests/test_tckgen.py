# Auto-generated test for tckgen

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import tckgen


def test_tckgen(tmp_path, cli_parse_only):

    task = tckgen(
        source=Nifti1.sample(),
        tracks=Tracks.sample(),
        algorithm="fact",
        select=1,
        step=1.0,
        angle=1.0,
        minlength=1.0,
        maxlength=1.0,
        cutoff=1.0,
        trials=1,
        noprecomputed=True,
        rk4=True,
        stop=True,
        downsample=1,
        seed_image=[Nifti1.sample()],
        seed_sphere=[list([1.0])],
        seed_random_per_voxel=[tuple([Nifti1.sample(), Nifti1.sample()])],
        seed_grid_per_voxel=[tuple([Nifti1.sample(), Nifti1.sample()])],
        seed_rejection=[Nifti1.sample()],
        seed_gmwmi=[Nifti1.sample()],
        seed_dynamic=Nifti1.sample(),
        seeds=1,
        max_attempts_per_seed=1,
        seed_cutoff=1.0,
        seed_unidirectional=True,
        seed_direction=list([1.0]),
        output_seeds=False,
        include=[File.sample()],
        include_ordered=["a-string"],
        exclude=[File.sample()],
        mask=[File.sample()],
        act=Nifti1.sample(),
        backtrack=True,
        crop_at_gmwmi=True,
        power=1.0,
        samples=1,
        grad=File.sample(),
        fslgrad=tuple([File.sample(), File.sample()]),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
