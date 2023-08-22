# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "type",
        str,
        {
            "argstr": "",
            "position": 0,
            "help_string": """type of histogram matching to perform; options are: scale,linear,nonlinear""",
            "mandatory": True,
            "allowed_values": ["scale", "scale", "linear", "nonlinear"],
        },
    ),
    (
        "input",
        ImageIn,
        {
            "argstr": "",
            "position": 1,
            "help_string": """the input image to be modified""",
            "mandatory": True,
        },
    ),
    (
        "target",
        ImageIn,
        {
            "argstr": "",
            "position": 2,
            "help_string": """the input image from which to derive the target histogram""",
            "mandatory": True,
        },
    ),
    (
        "output",
        Path,
        {
            "argstr": "",
            "position": 3,
            "output_file_template": "output.mif",
            "help_string": """the output image""",
        },
    ),
    # Image masking options Option Group
    (
        "mask_input",
        ImageIn,
        {
            "argstr": "-mask_input",
            "help_string": """only generate input histogram based on a specified binary mask image""",
        },
    ),
    (
        "mask_target",
        ImageIn,
        {
            "argstr": "-mask_target",
            "help_string": """only generate target histogram based on a specified binary mask image""",
        },
    ),
    # Non-linear histogram matching options Option Group
    (
        "bins",
        int,
        {
            "argstr": "-bins",
            "help_string": """the number of bins to use to generate the histograms""",
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

mrhistmatch_input_spec = specs.SpecInfo(
    name="mrhistmatch_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "output",
        ImageOut,
        {
            "help_string": """the output image""",
        },
    ),
]
mrhistmatch_output_spec = specs.SpecInfo(
    name="mrhistmatch_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class mrhistmatch(ShellCommandTask):
    """
        References
        ----------

            * If using inverse contrast normalization for inter-modal (DWI - T1) registration:
    Bhushan, C.; Haldar, J. P.; Choi, S.; Joshi, A. A.; Shattuck, D. W. & Leahy, R. M. Co-registration and distortion correction of diffusion and anatomical images based on inverse contrast normalization. NeuroImage, 2015, 115, 269-280

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: Robert E. Smith (robert.smith@florey.edu.au)

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

    executable = "mrhistmatch"
    input_spec = mrhistmatch_input_spec
    output_spec = mrhistmatch_output_spec
