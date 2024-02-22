# Auto-generated test for mrinfo

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import mrinfo


def test_mrinfo(tmp_path, cli_parse_only):

    task = mrinfo(
        image_=[Nifti1.sample()],
        all=True,
        name=True,
        format=True,
        ndim=True,
        size=True,
        spacing=True,
        datatype=True,
        strides=True,
        offset=True,
        multiplier=True,
        transform=True,
        property=["a-string"],
        json_keyval=False,
        json_all=False,
        grad=File.sample(),
        fslgrad=tuple([File.sample(), File.sample()]),
        bvalue_scaling=True,
        export_grad_mrtrix=False,
        export_grad_fsl=False,
        dwgrad=True,
        shell_bvalues=True,
        shell_sizes=True,
        shell_indices=True,
        export_pe_table=False,
        export_pe_eddy=False,
        petable=True,
        nodelete=True,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
