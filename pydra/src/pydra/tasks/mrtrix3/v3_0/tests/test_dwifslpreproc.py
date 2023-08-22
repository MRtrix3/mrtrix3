# Auto-generated test for dwifslpreproc

from fileformats.generic import File, Directory, FsObject  # noqa
from fileformats.medimage import Nifti1  # noqa
from fileformats.medimage_mrtrix3 import ImageFormat, ImageIn, Tracks  # noqa
from pydra.tasks.mrtrix3.v3_0 import dwifslpreproc


def test_dwifslpreproc(tmp_path, cli_parse_only):
    task = dwifslpreproc(
        input=File.sample(),
        output=FsObject.sample(),
        rpe_none=True,
        rpe_pair=True,
        rpe_all=True,
        rpe_header=True,
        grad=File.sample(),
        fslgrad=File.sample(),
        export_grad_mrtrix=File.sample(),
        export_grad_fsl=File.sample(),
        eddyqc_text=File.sample(),
        eddyqc_all=File.sample(),
        eddy_mask=File.sample(),
        eddy_slspec=File.sample(),
        eddy_options=File.sample(),
        se_epi=File.sample(),
        align_seepi=True,
        topup_options=File.sample(),
        topup_files=File.sample(),
        pe_dir=File.sample(),
        readout_time=1.0,
        nocleanup=True,
        scratch=File.sample(),
        cont=File.sample(),
        debug=True,
        force=True,
        json_import=File.sample(),
    )
    result = task(plugin="serial")
    assert not result.errored
