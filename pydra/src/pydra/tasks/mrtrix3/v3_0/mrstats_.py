# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "image_",
        ImageIn,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the input image from which statistics will be computed.""",
            "mandatory": True,
        },
    ),
    # Statistics options Option Group
    (
        "output",
        specs.MultiInputObj[str],
        {
            "argstr": "-output",
            "help_string": """output only the field specified. Multiple such options can be supplied if required. Choices are: mean, median, std, std_rv, min, max, count. Useful for use in scripts. Both std options refer to the unbiased (sample) standard deviation. For complex data, min, max and std are calculated separately for real and imaginary parts, std_rv is based on the real valued variance (equals sqrt of sum of variances of imaginary and real parts).""",
            "allowed_values": [
                "mean",
                "mean",
                "median",
                "std",
                "std_rv",
                "min",
                "max",
                "count",
            ],
        },
    ),
    (
        "mask",
        ImageIn,
        {
            "argstr": "-mask",
            "help_string": """only perform computation within the specified binary mask image.""",
        },
    ),
    (
        "ignorezero",
        bool,
        {
            "argstr": "-ignorezero",
            "help_string": """ignore zero values during statistics calculation""",
        },
    ),
    # Additional options for mrstats Option Group
    (
        "allvolumes",
        bool,
        {
            "argstr": "-allvolumes",
            "help_string": """generate statistics across all image volumes, rather than one set of statistics per image volume""",
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

mrstats_input_spec = specs.SpecInfo(
    name="mrstats_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = []
mrstats_output_spec = specs.SpecInfo(
    name="mrstats_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class mrstats(ShellCommandTask):
    """
        References
        ----------

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: J-Donald Tournier (jdtournier@gmail.com)

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

    executable = "mrstats"
    input_spec = mrstats_input_spec
    output_spec = mrstats_output_spec
