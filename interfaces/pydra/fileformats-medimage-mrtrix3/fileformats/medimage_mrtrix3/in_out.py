import typing as ty
from fileformats.application import Dicom
from fileformats.medimage import (
    DicomDir,
    NiftiGzX,
    NiftiGz,
    NiftiX,
    Nifti1,
    Nifti2,
    Mgh,
    MghGz,
    Analyze,
)
from .image import ImageFormat, ImageHeader, ImageFormatGz
from .dwi import (
    NiftiB, NiftiGzB, NiftiGzXB, NiftiXB, ImageFormatB, ImageFormatGzB, ImageHeaderB
)


ImageIn = ty.Union[
    ImageFormat,
    ImageFormatGz,
    ImageHeader,
    ImageFormatB,
    ImageFormatGzB,
    ImageHeaderB,
    Dicom,
    DicomDir,
    NiftiGzX,
    NiftiGz,
    NiftiX,
    Nifti1,
    Nifti2,
    Mgh,
    MghGz,
    Analyze,
]

ImageOut = ty.Union[
    ImageFormat,
    ImageFormatGz,
    ImageHeader,
    ImageFormatB,
    ImageFormatGzB,
    ImageHeaderB,
    NiftiGzX,
    NiftiGz,
    NiftiX,
    Nifti1,
    Nifti2,
    Mgh,
    MghGz,
    Analyze,
]
