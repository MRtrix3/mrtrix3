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
            "help_string": """the input image.""",
            "mandatory": True,
        },
    ),
    (
        "filter",
        str,
        {
            "argstr": "",
            "position": 1,
            "help_string": """the type of filter to be applied""",
            "mandatory": True,
            "allowed_values": [
                "fft",
                "fft",
                "gradient",
                "median",
                "smooth",
                "normalise",
                "zclean",
            ],
        },
    ),
    (
        "output",
        Path,
        {
            "argstr": "",
            "position": 2,
            "output_file_template": "output.mif",
            "help_string": """the output image.""",
        },
    ),
    # Options for FFT filter Option Group
    (
        "axes",
        ty.List[int],
        {
            "argstr": "-axes",
            "help_string": """the axes along which to apply the Fourier Transform. By default, the transform is applied along the three spatial axes. Provide as a comma-separate list of axis indices.""",
            "sep": ",",
        },
    ),
    (
        "inverse",
        bool,
        {
            "argstr": "-inverse",
            "help_string": """apply the inverse FFT""",
        },
    ),
    (
        "magnitude",
        bool,
        {
            "argstr": "-magnitude",
            "help_string": """output a magnitude image rather than a complex-valued image""",
        },
    ),
    (
        "rescale",
        bool,
        {
            "argstr": "-rescale",
            "help_string": """rescale values so that inverse FFT recovers original values""",
        },
    ),
    (
        "centre_zero",
        bool,
        {
            "argstr": "-centre_zero",
            "help_string": """re-arrange the FFT results so that the zero-frequency component appears in the centre of the image, rather than at the edges""",
        },
    ),
    # Options for gradient filter Option Group
    (
        "stdev",
        ty.List[float],
        {
            "argstr": "-stdev",
            "help_string": """the standard deviation of the Gaussian kernel used to smooth the input image (in mm). The image is smoothed to reduced large spurious gradients caused by noise. Use this option to override the default stdev of 1 voxel. This can be specified either as a single value to be used for all 3 axes, or as a comma-separated list of 3 values, one for each axis.""",
            "sep": ",",
        },
    ),
    (
        "magnitude",
        bool,
        {
            "argstr": "-magnitude",
            "help_string": """output the gradient magnitude, rather than the default x,y,z components""",
        },
    ),
    (
        "scanner",
        bool,
        {
            "argstr": "-scanner",
            "help_string": """define the gradient with respect to the scanner coordinate frame of reference.""",
        },
    ),
    # Options for median filter Option Group
    (
        "extent",
        ty.List[int],
        {
            "argstr": "-extent",
            "help_string": """specify extent of median filtering neighbourhood in voxels. This can be specified either as a single value to be used for all 3 axes, or as a comma-separated list of 3 values, one for each axis (default: 3x3x3).""",
            "sep": ",",
        },
    ),
    # Options for normalisation filter Option Group
    (
        "extent",
        ty.List[int],
        {
            "argstr": "-extent",
            "help_string": """specify extent of normalisation filtering neighbourhood in voxels. This can be specified either as a single value to be used for all 3 axes, or as a comma-separated list of 3 values, one for each axis (default: 3x3x3).""",
            "sep": ",",
        },
    ),
    # Options for smooth filter Option Group
    (
        "stdev",
        ty.List[float],
        {
            "argstr": "-stdev",
            "help_string": """apply Gaussian smoothing with the specified standard deviation. The standard deviation is defined in mm (Default 1 voxel). This can be specified either as a single value to be used for all axes, or as a comma-separated list of the stdev for each axis.""",
            "sep": ",",
        },
    ),
    (
        "fwhm",
        ty.List[float],
        {
            "argstr": "-fwhm",
            "help_string": """apply Gaussian smoothing with the specified full-width half maximum. The FWHM is defined in mm (Default 1 voxel * 2.3548). This can be specified either as a single value to be used for all axes, or as a comma-separated list of the FWHM for each axis.""",
            "sep": ",",
        },
    ),
    (
        "extent",
        ty.List[int],
        {
            "argstr": "-extent",
            "help_string": """specify the extent (width) of kernel size in voxels. This can be specified either as a single value to be used for all axes, or as a comma-separated list of the extent for each axis. The default extent is 2 * ceil(2.5 * stdev / voxel_size) - 1.""",
            "sep": ",",
        },
    ),
    # Options for zclean filter Option Group
    (
        "zupper",
        float,
        {
            "argstr": "-zupper",
            "help_string": """define high intensity outliers: default: 2.5""",
        },
    ),
    (
        "zlower",
        float,
        {
            "argstr": "-zlower",
            "help_string": """define low intensity outliers: default: 2.5""",
        },
    ),
    (
        "bridge",
        int,
        {
            "argstr": "-bridge",
            "help_string": """number of voxels to gap to fill holes in mask: default: 4""",
        },
    ),
    (
        "maskin",
        ImageIn,
        {
            "argstr": "-maskin",
            "help_string": """initial mask that defines the maximum spatial extent and the region from which to smaple the intensity range.""",
        },
    ),
    (
        "maskout",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-maskout",
            "output_file_template": "maskout.mif",
            "help_string": """Output a refined mask based on a spatially coherent region with normal intensity range.""",
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

mrfilter_input_spec = specs.SpecInfo(
    name="mrfilter_input", fields=input_fields, bases=(specs.ShellSpec,)
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
        "maskout",
        ImageOut,
        {
            "help_string": """Output a refined mask based on a spatially coherent region with normal intensity range.""",
        },
    ),
]
mrfilter_output_spec = specs.SpecInfo(
    name="mrfilter_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class mrfilter(ShellCommandTask):
    """The available filters are: fft, gradient, median, smooth, normalise, zclean.

        Each filter has its own unique set of optional parameters.

        For 4D images, each 3D volume is processed independently.


        References
        ----------

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: Robert E. Smith (robert.smith@florey.edu.au), David Raffelt (david.raffelt@florey.edu.au) and J-Donald Tournier (jdtournier@gmail.com)

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

    executable = "mrfilter"
    input_spec = mrfilter_input_spec
    output_spec = mrfilter_output_spec
