# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "input",
        ImageIn,
        {
            "argstr": "",
            "position": 0,
            "help_string": """input image to be transformed.""",
            "mandatory": True,
        },
    ),
    (
        "output",
        Path,
        {
            "argstr": "",
            "position": 1,
            "output_file_template": "output.mif",
            "help_string": """the output image.""",
        },
    ),
    # Affine transformation options Option Group
    (
        "linear",
        File,
        {
            "argstr": "-linear",
            "help_string": """specify a linear transform to apply, in the form of a 3x4 or 4x4 ascii file. Note the standard 'reverse' convention is used, where the transform maps points in the template image to the moving image. Note that the reverse convention is still assumed even if no -template image is supplied""",
        },
    ),
    (
        "flip",
        ty.List[int],
        {
            "argstr": "-flip",
            "help_string": """flip the specified axes, provided as a comma-separated list of indices (0:x, 1:y, 2:z).""",
            "sep": ",",
        },
    ),
    (
        "inverse",
        bool,
        {
            "argstr": "-inverse",
            "help_string": """apply the inverse transformation""",
        },
    ),
    (
        "half",
        bool,
        {
            "argstr": "-half",
            "help_string": """apply the matrix square root of the transformation. This can be combined with the inverse option.""",
        },
    ),
    (
        "replace",
        File,
        {
            "argstr": "-replace",
            "help_string": """replace the linear transform of the original image by that specified, rather than applying it to the original image. The specified transform can be either a template image, or a 3x4 or 4x4 ascii file.""",
        },
    ),
    (
        "identity",
        bool,
        {
            "argstr": "-identity",
            "help_string": """set the header transform of the image to the identity matrix""",
        },
    ),
    # Regridding options Option Group
    (
        "template",
        ImageIn,
        {
            "argstr": "-template",
            "help_string": """reslice the input image to match the specified template image grid.""",
        },
    ),
    (
        "midway_space",
        bool,
        {
            "argstr": "-midway_space",
            "help_string": """reslice the input image to the midway space. Requires either the -template or -warp option. If used with -template and -linear option the input image will be resliced onto the grid halfway between the input and template. If used with the -warp option the input will be warped to the midway space defined by the grid of the input warp (i.e. half way between image1 and image2)""",
        },
    ),
    (
        "interp",
        str,
        {
            "argstr": "-interp",
            "help_string": """set the interpolation method to use when reslicing (choices: nearest, linear, cubic, sinc. Default: cubic).""",
            "allowed_values": ["nearest", "nearest", "linear", "cubic", "sinc"],
        },
    ),
    (
        "oversample",
        ty.List[int],
        {
            "argstr": "-oversample",
            "help_string": """set the amount of over-sampling (in the target space) to perform when regridding. This is particularly relevant when downsamping a high-resolution image to a low-resolution image, to avoid aliasing artefacts. This can consist of a single integer, or a comma-separated list of 3 integers if different oversampling factors are desired along the different axes. Default is determined from ratio of voxel dimensions (disabled for nearest-neighbour interpolation).""",
            "sep": ",",
        },
    ),
    # Non-linear transformation options Option Group
    (
        "warp",
        ImageIn,
        {
            "argstr": "-warp",
            "help_string": """apply a non-linear 4D deformation field to warp the input image. Each voxel in the deformation field must define the scanner space position that will be used to interpolate the input image during warping (i.e. pull-back/reverse warp convention). If the -template image is also supplied the deformation field will be resliced first to the template image grid. If no -template option is supplied then the output image will have the same image grid as the deformation field. This option can be used in combination with the -affine option, in which case the affine will be applied first)""",
        },
    ),
    (
        "warp_full",
        ImageIn,
        {
            "argstr": "-warp_full",
            "help_string": """warp the input image using a 5D warp file output from mrregister. Any linear transforms in the warp image header will also be applied. The -warp_full option must be used in combination with either the -template option or the -midway_space option. If a -template image is supplied then the full warp will be used. By default the image1->image2 transform will be applied, however the -from 2 option can be used to apply the image2->image1 transform. Use the -midway_space option to warp the input image to the midway space. The -from option can also be used to define which warp to use when transforming to midway space""",
        },
    ),
    (
        "from_",
        int,
        {
            "argstr": "-from",
            "help_string": """used to define which space the input image is when using the -warp_mid option. Use -from 1 to warp from image1 or -from 2 to warp from image2""",
        },
    ),
    # Fibre orientation distribution handling options Option Group
    (
        "modulate",
        str,
        {
            "argstr": "-modulate",
            "help_string": """Valid choices are: fod and jac. 
fod: modulate FODs during reorientation to preserve the apparent fibre density across fibre bundle widths before and after the transformation. 
jac: modulate the image intensity with the determinant of the Jacobian of the warp of linear transformation to preserve the total intensity before and after the transformation.""",
            "allowed_values": ["fod", "fod", "jac"],
        },
    ),
    (
        "directions",
        File,
        {
            "argstr": "-directions",
            "help_string": """directions defining the number and orientation of the apodised point spread functions used in FOD reorientation (Default: 300 directions)""",
        },
    ),
    (
        "reorient_fod",
        bool,
        {
            "argstr": "-reorient_fod",
            "help_string": """specify whether to perform FOD reorientation. This is required if the number of volumes in the 4th dimension corresponds to the number of coefficients in an antipodally symmetric spherical harmonic series with lmax >= 2 (i.e. 6, 15, 28, 45, 66 volumes).""",
        },
    ),
    # DW gradient table import options Option Group
    (
        "grad",
        File,
        {
            "argstr": "-grad",
            "help_string": """Provide the diffusion-weighted gradient scheme used in the acquisition in a text file. This should be supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.""",
        },
    ),
    (
        "fslgrad",
        ty.Tuple[File, File],
        {
            "argstr": "-fslgrad",
            "help_string": """Provide the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format files. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.""",
        },
    ),
    # DW gradient table export options Option Group
    (
        "export_grad_mrtrix",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-export_grad_mrtrix",
            "output_file_template": "export_grad_mrtrix.txt",
            "help_string": """export the diffusion-weighted gradient table to file in MRtrix format""",
        },
    ),
    (
        "export_grad_fsl",
        ty.Union[ty.Tuple[Path, Path], bool],
        False,
        {
            "argstr": "-export_grad_fsl",
            "output_file_template": (
                "export_grad_fsl0.txt",
                "export_grad_fsl1.txt",
            ),
            "help_string": """export the diffusion-weighted gradient table to files in FSL (bvecs / bvals) format""",
        },
    ),
    # Data type options Option Group
    (
        "datatype",
        str,
        {
            "argstr": "-datatype",
            "help_string": """specify output image data type. Valid choices are: float16, float16le, float16be, float32, float32le, float32be, float64, float64le, float64be, int64, uint64, int64le, uint64le, int64be, uint64be, int32, uint32, int32le, uint32le, int32be, uint32be, int16, uint16, int16le, uint16le, int16be, uint16be, cfloat16, cfloat16le, cfloat16be, cfloat32, cfloat32le, cfloat32be, cfloat64, cfloat64le, cfloat64be, int8, uint8, bit.""",
            "allowed_values": [
                "float16",
                "float16",
                "float16le",
                "float16be",
                "float32",
                "float32le",
                "float32be",
                "float64",
                "float64le",
                "float64be",
                "int64",
                "uint64",
                "int64le",
                "uint64le",
                "int64be",
                "uint64be",
                "int32",
                "uint32",
                "int32le",
                "uint32le",
                "int32be",
                "uint32be",
                "int16",
                "uint16",
                "int16le",
                "uint16le",
                "int16be",
                "uint16be",
                "cfloat16",
                "cfloat16le",
                "cfloat16be",
                "cfloat32",
                "cfloat32le",
                "cfloat32be",
                "cfloat64",
                "cfloat64le",
                "cfloat64be",
                "int8",
                "uint8",
                "bit",
            ],
        },
    ),
    # Stride options Option Group
    (
        "strides",
        ty.Any,
        {
            "argstr": "-strides",
            "help_string": """specify the strides of the output data in memory; either as a comma-separated list of (signed) integers, or as a template image from which the strides shall be extracted and used. The actual strides produced will depend on whether the output image format can support it.""",
        },
    ),
    # Additional generic options for mrtransform Option Group
    (
        "nan",
        bool,
        {
            "argstr": "-nan",
            "help_string": """Use NaN as the out of bounds value (Default: 0.0)""",
        },
    ),
    (
        "no_reorientation",
        bool,
        {
            "argstr": "-no_reorientation",
            "help_string": """deprecated, use -reorient_fod instead""",
        },
    ),
    # Standard options
    (
        "info",
        bool,
        {
            "argstr": "-info",
            "help_string": """display information messages.""",
        },
    ),
    (
        "quiet",
        bool,
        {
            "argstr": "-quiet",
            "help_string": """do not display information messages or progress status; alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.""",
        },
    ),
    (
        "debug",
        bool,
        {
            "argstr": "-debug",
            "help_string": """display debugging messages.""",
        },
    ),
    (
        "force",
        bool,
        {
            "argstr": "-force",
            "help_string": """force overwrite of output files (caution: using the same file as input and output might cause unexpected behaviour).""",
        },
    ),
    (
        "nthreads",
        int,
        {
            "argstr": "-nthreads",
            "help_string": """use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).""",
        },
    ),
    (
        "config",
        specs.MultiInputObj[ty.Tuple[str, str]],
        {
            "argstr": "-config",
            "help_string": """temporarily set the value of an MRtrix config file entry.""",
        },
    ),
    (
        "help",
        bool,
        {
            "argstr": "-help",
            "help_string": """display this information page and exit.""",
        },
    ),
    (
        "version",
        bool,
        {
            "argstr": "-version",
            "help_string": """display version information and exit.""",
        },
    ),
]

mrtransform_input_spec = specs.SpecInfo(
    name="mrtransform_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "output",
        ImageOut,
        {
            "help_string": """the output image.""",
        },
    ),
    (
        "export_grad_mrtrix",
        File,
        {
            "help_string": """export the diffusion-weighted gradient table to file in MRtrix format""",
        },
    ),
    (
        "export_grad_fsl",
        ty.Tuple[File, File],
        {
            "help_string": """export the diffusion-weighted gradient table to files in FSL (bvecs / bvals) format""",
        },
    ),
]
mrtransform_output_spec = specs.SpecInfo(
    name="mrtransform_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class mrtransform(ShellCommandTask):
    """If a linear transform is applied without a template image the command will modify the image header transform matrix

        FOD reorientation (with apodised point spread functions) can be performed if the number of volumes in the 4th dimension equals the number of coefficients in an antipodally symmetric spherical harmonic series (e.g. 6, 15, 28 etc). For such data, the -reorient_fod yes/no option must be used to specify if reorientation is required.

        The output image intensity can be modulated using the (local or global) volume change if a linear or nonlinear transformation is applied. 'FOD' modulation preserves the apparent fibre density across the fibre bundle width and can only be applied if FOD reorientation is used. Alternatively, non-directional scaling by the Jacobian determinant can be applied to any image type.

        If a DW scheme is contained in the header (or specified separately), and the number of directions matches the number of volumes in the images, any transformation applied using the -linear option will also be applied to the directions.

        When the -template option is used to specify the target image grid, the image provided via this option will not influence the axis data strides of the output image; these are determined based on the input image, or the input to the -strides option.


        References
        ----------

            * If FOD reorientation is being performed:
    Raffelt, D.; Tournier, J.-D.; Crozier, S.; Connelly, A. & Salvado, O. Reorientation of fiber orientation distributions using apodized point spread functions. Magnetic Resonance in Medicine, 2012, 67, 844-855

            * If FOD modulation is being performed:
    Raffelt, D.; Tournier, J.-D.; Rose, S.; Ridgway, G.R.; Henderson, R.; Crozier, S.; Salvado, O.; Connelly, A.; Apparent Fibre Density: a novel measure for the analysis of diffusion-weighted magnetic resonance images. NeuroImage, 2012, 15;59(4), 3976-94

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: J-Donald Tournier (jdtournier@gmail.com) and David Raffelt (david.raffelt@florey.edu.au) and Max Pietsch (maximilian.pietsch@kcl.ac.uk)

            Copyright: Copyright (c) 2008-2023 the MRtrix3 contributors.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

    Covered Software is provided under this License on an "as is"
    basis, without warranty of any kind, either expressed, implied, or
    statutory, including, without limitation, warranties that the
    Covered Software is free of defects, merchantable, fit for a
    particular purpose or non-infringing.
    See the Mozilla Public License v. 2.0 for more details.

    For more details, see http://www.mrtrix.org/.
    """

    executable = "mrtransform"
    input_spec = mrtransform_input_spec
    output_spec = mrtransform_output_spec
