from fileformats.core import mark
from fileformats.medimage.base import MedicalImage

from fileformats.mrtrix3 import ImageFormat as MrtrixImage, ImageHeader as MrtrixImageHeader
try:
    from pydra.tasks.mrtrix3.utils import MRConvert
except ImportError:
    from pydra.tasks.mrtrix3.latest import mrconvert as MRConvert


@mark.converter(
    source_format=MedicalImage, target_format=MrtrixImage, out_ext=MrtrixImage.ext
)
@mark.converter(
    source_format=MedicalImage,
    target_format=MrtrixImageHeader,
    out_ext=MrtrixImageHeader.ext,
)
def mrconvert(name, out_ext: str):
    """Initiate an MRConvert task with the output file extension set

    Parameters
    ----------
    name : str
        name of the converter task
    out_ext : str
        extension of the output file, used by MRConvert to determine the desired format

    Returns
    -------
    pydra.ShellCommandTask
        the converter task
    """
    return MRConvert(name=name, out_file="out" + out_ext)
