# Auto-generated test for mrconvert

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import mrconvert


def test_mrconvert(tmp_path, cli_parse_only):
    task = mrconvert(
        input=Nifti1.sample(),
        output=ImageFormat.sample(),
        coord=[tuple([1, 1])],
        vox=list([1.0]),
        axes=list([1]),
        scaling=list([1.0]),
        json_import=File.sample(),
        json_export=False,
        clear_property=["a-string"],
        set_property=[tuple(["a-string", "a-string"])],
        append_property=[tuple(["a-string", "a-string"])],
        copy_properties=File.sample(),
        strides=File.sample(),
        datatype="float16",
        grad=File.sample(),
        fslgrad=tuple([File.sample(), File.sample()]),
        bvalue_scaling=True,
        export_grad_mrtrix=False,
        export_grad_fsl=False,
        import_pe_table=File.sample(),
        import_pe_eddy=tuple([File.sample(), File.sample()]),
        export_pe_table=False,
        export_pe_eddy=False,
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
