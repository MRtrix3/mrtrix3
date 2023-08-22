# Auto-generated test for dwiextract

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import dwiextract


def test_dwiextract(tmp_path, cli_parse_only):
    task = dwiextract(
        input=Nifti1.sample(),
        output=ImageFormat.sample(),
        bzero=True,
        no_bzero=True,
        singleshell=True,
        grad=File.sample(),
        fslgrad=tuple([File.sample(), File.sample()]),
        shells=list([1.0]),
        export_grad_mrtrix=False,
        export_grad_fsl=False,
        import_pe_table=File.sample(),
        import_pe_eddy=tuple([File.sample(), File.sample()]),
        pe=list([1.0]),
        strides=File.sample(),
        debug=True,
        force=True,
    )
    result = task(plugin="serial")
    assert not result.errored
