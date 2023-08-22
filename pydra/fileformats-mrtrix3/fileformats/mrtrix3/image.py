from pathlib import Path
import typing as ty
from fileformats.core import mark
from fileformats.generic import File
from fileformats.core.mixin import WithMagicNumber
from fileformats.core.exceptions import FormatMismatchError
import fileformats.medimage


class BaseMrtrixImage(WithMagicNumber, fileformats.medimage.MedicalImage, File):

    magic_number = b"mrtrix image\n"
    binary = True

    def read_metadata(self):
        metadata = {}
        with open(self.fspath, "rb") as f:
            line = f.readline()
            if line != self.magic_number:
                raise FormatMismatchError(
                    f"Magic line {line} doesn't match reference {self.magic_number}"
                )
            line = f.readline().decode("utf-8")
            while line and line != "END\n":
                key, value = line.split(": ", maxsplit=1)
                if "," in value:
                    try:
                        value = [int(v) for v in value.split(",")]
                    except ValueError:
                        try:
                            value = [float(v) for v in value.split(",")]
                        except ValueError:
                            pass
                else:
                    try:
                        value = int(value)
                    except ValueError:
                        try:
                            value = float(value)
                        except ValueError:
                            pass
                metadata[key] = value
                line = f.readline().decode("utf-8")
        return metadata

    @property
    def data_fspath(self):
        data_fspath = self.metadata["file"].split()[0]
        if data_fspath == ".":
            data_fspath = self.fspath
        elif Path(data_fspath).parent == Path("."):
            data_fspath = self.fspath.parent / data_fspath
        else:
            data_fspath = Path(data_fspath).relative_to(self.fspath.parent)
        return data_fspath

    @property
    def data_offset(self):
        return int(self.metadata["file"].split()[1])

    @property
    def vox_sizes(self):
        return self.metadata["vox"]

    @property
    def dims(self):
        return self.metadata["dim"]


class ImageFormat(BaseMrtrixImage):

    ext = ".mif"

    @mark.check
    def check_data_file(self):
        if self.data_fspath != self.fspath:
            raise FormatMismatchError(
                f"Data file ('{self.data_fspath}') is not set to the same file as header "
                f"('{self.fspath}')")

    @property
    def data_file(self):
        return self


class ImageHeader(BaseMrtrixImage):

    ext = ".mih"

    @mark.required
    @property
    def data_file(self):
        return ImageDataFile(self.data_fspath)

    def __attrs_post_init__(self):
        if len(self.fspaths) == 1:
            # add in data file if only header file is provided
            self.fspaths |= set([BaseMrtrixImage(self.fspath).data_fspath])
        super().__attrs_post_init__()


class ImageDataFile(File):

    ext = ".dat"


ImageIn = ty.Union[
    ImageFormat,
    ImageHeader,
    fileformats.misc.Dicom,
    fileformats.medimage.DicomDir,
    fileformats.medimage.NiftiGzX,
    fileformats.medimage.NiftiGz,
    fileformats.medimage.NiftiX,
    fileformats.medimage.Nifti1,
    fileformats.medimage.Nifti2,
    fileformats.medimage.Mgh,
    fileformats.medimage.MghZip,
    fileformats.medimage.Analyze,
]

ImageOut = ty.Union[
    ImageFormat,
    ImageHeader,
    fileformats.medimage.NiftiGzX,
    fileformats.medimage.NiftiGz,
    fileformats.medimage.NiftiX,
    fileformats.medimage.Nifti1,
    fileformats.medimage.Nifti2,
    fileformats.medimage.Mgh,
    fileformats.medimage.MghZip,
    fileformats.medimage.Analyze,
]
