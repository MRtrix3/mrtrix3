import os
from unittest import mock
import typing as ty
from pathlib import Path
import numpy as np
from random import Random
from medimages4tests.dummy.nifti import get_image as get_dummy_nifti
from fileformats.core import FileSet
from fileformats.medimage import MedicalImage, Nifti1
from fileformats.medimage_mrtrix3 import ImageFormat


@FileSet.generate_sample_data.register
def generate_mrtrix_sample_data(mif: ImageFormat, dest_dir: Path, seed: ty.Union[int, Random] = 0, stem: ty.Optional[str] = None) -> ty.Iterable[Path]:
    nifti = Nifti1(get_dummy_nifti(dest_dir / "nifti.nii"))
    with mock.patch.dict(os.environ, {"MRTRIX_CLI_PARSE_ONLY": "0"}):
        mif = ImageFormat.convert(nifti)
    return mif.fspaths


@MedicalImage.read_array.register
def mrtrix_read_array(mif: ImageFormat) -> np.ndarray:
    raise NotImplementedError(
        "Need to work out how to use the metadata to read the array in the correct order"
    )
    data = mif.read_contents(offset=mif.data_offset)
    array = np.asarray(data)
    data_array = array.reshape(mif.dims)
    return data_array
