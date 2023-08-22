import typing as ty
from fileformats.medimage import DicomDir, Nifti, MrtrixImage, Analyze

ImageIn = ty.Union[DicomDir, Nifti, MrtrixImage, Analyze]
ImageOut = ty.Union[Nifti, MrtrixImage, Analyze]
