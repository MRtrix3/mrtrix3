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
            "help_string": """the input image to be thresholded""",
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
            "help_string": """the (optional) output binary image mask""",
        },
    ),
    # Threshold determination mechanisms Option Group
    (
        "abs",
        float,
        {
            "argstr": "-abs",
            "help_string": """specify threshold value as absolute intensity""",
        },
    ),
    (
        "percentile",
        float,
        {
            "argstr": "-percentile",
            "help_string": """determine threshold based on some percentile of the image intensity distribution""",
        },
    ),
    (
        "top",
        int,
        {
            "argstr": "-top",
            "help_string": """determine threshold that will result in selection of some number of top-valued voxels""",
        },
    ),
    (
        "bottom",
        int,
        {
            "argstr": "-bottom",
            "help_string": """determine & apply threshold resulting in selection of some number of bottom-valued voxels (note: implies threshold application operator of "le" unless otherwise specified)""",
        },
    ),
    # Threshold determination modifiers Option Group
    (
        "allvolumes",
        bool,
        {
            "argstr": "-allvolumes",
            "help_string": """compute a single threshold for all image volumes, rather than an individual threshold per volume""",
        },
    ),
    (
        "ignorezero",
        bool,
        {
            "argstr": "-ignorezero",
            "help_string": """ignore zero-valued input values during threshold determination""",
        },
    ),
    (
        "mask",
        ImageIn,
        {
            "argstr": "-mask",
            "help_string": """compute the threshold based only on values within an input mask image""",
        },
    ),
    # Threshold application modifiers Option Group
    (
        "comparison",
        str,
        {
            "argstr": "-comparison",
            "help_string": """comparison operator to use when applying the threshold; options are: lt,le,ge,gt (default = "le" for -bottom; "ge" otherwise)""",
            "allowed_values": ["lt", "lt", "le", "ge", "gt"],
        },
    ),
    (
        "invert",
        bool,
        {
            "argstr": "-invert",
            "help_string": """invert the output binary mask (equivalent to flipping the operator; provided for backwards compatibility)""",
        },
    ),
    (
        "out_masked",
        bool,
        {
            "argstr": "-out_masked",
            "help_string": """mask the output image based on the provided input mask image""",
        },
    ),
    (
        "nan",
        bool,
        {
            "argstr": "-nan",
            "help_string": """set voxels that fail the threshold to NaN rather than zero (output image will be floating-point rather than binary)""",
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

mrthreshold_input_spec = specs.SpecInfo(
    name="mrthreshold_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "output",
        ImageOut,
        {
            "help_string": """the (optional) output binary image mask""",
        },
    ),
]
mrthreshold_output_spec = specs.SpecInfo(
    name="mrthreshold_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class mrthreshold(ShellCommandTask):
    """The threshold value to be applied can be determined in one of a number of ways:

        - If no relevant command-line option is used, the command will automatically determine an optimal threshold;

        - The -abs option provides the threshold value explicitly;

        - The -percentile, -top and -bottom options enable more fine-grained control over how the threshold value is determined.

        The -mask option only influences those image values that contribute toward the determination of the threshold value; once the threshold is determined, it is applied to the entire image, irrespective of use of the -mask option. If you wish for the voxels outside of the specified mask to additionally be excluded from the output mask, this can be achieved by providing the -out_masked option.

        The four operators available through the "-comparison" option ("lt", "le", "ge" and "gt") correspond to "less-than" (<), "less-than-or-equal" (<=), "greater-than-or-equal" (>=) and "greater-than" (>). This offers fine-grained control over how the thresholding operation will behave in the presence of values equivalent to the threshold. By default, the command will select voxels with values greater than or equal to the determined threshold ("ge"); unless the -bottom option is used, in which case after a threshold is determined from the relevant lowest-valued image voxels, those voxels with values less than or equal to that threshold ("le") are selected. This provides more fine-grained control than the -invert option; the latter is provided for backwards compatibility, but is equivalent to selection of the opposite comparison within this selection.

        If no output image path is specified, the command will instead write to standard output the determined threshold value.


        References
        ----------

            * If not using any explicit thresholding mechanism:
    Ridgway, G. R.; Omar, R.; Ourselin, S.; Hill, D. L.; Warren, J. D. & Fox, N. C. Issues with threshold masking in voxel-based morphometry of atrophied brains. NeuroImage, 2009, 44, 99-111

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: Robert E. Smith (robert.smith@florey.edu.au) and J-Donald Tournier (jdtournier@gmail.com)

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

    executable = "mrthreshold"
    input_spec = mrthreshold_input_spec
    output_spec = mrthreshold_output_spec
