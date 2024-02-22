from fileformats.medimage import NiftiBvec
from fileformats.medimage_mrtrix3 import ImageFormat, ImageHeader


# @pytest.mark.xfail(reason="not sure what the reason is at this stage, might be bug in Pydra")
def test_nifti_to_mrtrix(dummy_dwi_dicom):
    nifti_fsgrad = NiftiBvec.convert(dummy_dwi_dicom)
    ImageFormat.convert(nifti_fsgrad)
    ImageHeader.convert(nifti_fsgrad)


def test_dicom_to_mrtrix_image(dummy_dwi_dicom):
    ImageFormat.convert(dummy_dwi_dicom)


def test_dicom_to_mrtrix_image_header(dummy_dwi_dicom):
    ImageHeader.convert(dummy_dwi_dicom)
