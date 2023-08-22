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
            "help_string": """the input mask.""",
            "mandatory": True,
        },
    ),
    (
        "filter",
        str,
        {
            "argstr": "",
            "position": 1,
            "help_string": """the name of the filter to be applied; options are: clean, connect, dilate, erode, fill, median""",
            "mandatory": True,
            "allowed_values": [
                "clean",
                "clean",
                "connect",
                "dilate",
                "erode",
                "fill",
                "median",
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
            "help_string": """the output mask.""",
        },
    ),
    # Options for mask cleaning filter Option Group
    (
        "scale",
        int,
        {
            "argstr": "-scale",
            "help_string": """the maximum scale used to cut bridges. A certain maximum scale cuts bridges up to a width (in voxels) of 2x the provided scale. (Default: 2)""",
        },
    ),
    # Options for connected-component filter Option Group
    (
        "axes",
        ty.List[int],
        {
            "argstr": "-axes",
            "help_string": """specify which axes should be included in the connected components. By default only the first 3 axes are included. The axes should be provided as a comma-separated list of values.""",
            "sep": ",",
        },
    ),
    (
        "largest",
        bool,
        {
            "argstr": "-largest",
            "help_string": """only retain the largest connected component""",
        },
    ),
    (
        "connectivity",
        bool,
        {
            "argstr": "-connectivity",
            "help_string": """use 26-voxel-neighbourhood connectivity (Default: 6)""",
        },
    ),
    (
        "minsize",
        int,
        {
            "argstr": "-minsize",
            "help_string": """impose minimum size of segmented components (Default: select all components)""",
        },
    ),
    # Options for dilate / erode filters Option Group
    (
        "npass",
        int,
        {
            "argstr": "-npass",
            "help_string": """the number of times to repeatedly apply the filter""",
        },
    ),
    # Options for interior-filling filter Option Group
    (
        "axes",
        ty.List[int],
        {
            "argstr": "-axes",
            "help_string": """specify which axes should be included in the connected components. By default only the first 3 axes are included. The axes should be provided as a comma-separated list of values.""",
            "sep": ",",
        },
    ),
    (
        "connectivity",
        bool,
        {
            "argstr": "-connectivity",
            "help_string": """use 26-voxel-neighbourhood connectivity (Default: 6)""",
        },
    ),
    # Options for median filter Option Group
    (
        "extent",
        ty.List[int],
        {
            "argstr": "-extent",
            "help_string": """specify the extent (width) of kernel size in voxels. This can be specified either as a single value to be used for all axes, or as a comma-separated list of the extent for each axis. The default is 3x3x3.""",
            "sep": ",",
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

maskfilter_input_spec = specs.SpecInfo(
    name="maskfilter_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "output",
        ImageOut,
        {
            "help_string": """the output mask.""",
        },
    ),
]
maskfilter_output_spec = specs.SpecInfo(
    name="maskfilter_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class maskfilter(ShellCommandTask):
    """Many filters have their own unique set of optional parameters; see the option groups dedicated to each filter type.


        References
        ----------

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: Robert E. Smith (robert.smith@florey.edu.au), David Raffelt (david.raffelt@florey.edu.au), Thijs Dhollander (thijs.dhollander@gmail.com) and J-Donald Tournier (jdtournier@gmail.com)

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

    executable = "maskfilter"
    input_spec = maskfilter_input_spec
    output_spec = maskfilter_output_spec
