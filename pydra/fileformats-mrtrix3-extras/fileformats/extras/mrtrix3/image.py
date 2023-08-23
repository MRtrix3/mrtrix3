import numpy as np
from fileformats.medimage import MedicalImage, MrtrixImage


@MedicalImage.read_array.register
def mrtrix_read_array(mif: MrtrixImage):
    raise NotImplementedError(
        "Need to work out how to use the metadata to read the array in the correct order"
    )
    data = mif.read_contents(offset=mif.data_offset)
    array = np.asarray(data)
    data_array = array.reshape(mif.dims)
    return data_array
