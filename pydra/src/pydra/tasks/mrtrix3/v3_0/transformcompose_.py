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
        specs.MultiInputObj[File],
        {
            "argstr": "",
            "position": 0,
            "help_string": """the input transforms (either linear or non-linear warps).""",
            "mandatory": True,
        },
    ),
    (
        "output",
        ty.Any,
        {
            "argstr": "",
            "position": 1,
            "help_string": """the output file (may be a linear transformation text file, or a deformation warp field image, depending on usage)""",
            "mandatory": True,
        },
    ),
    (
        "template",
        ImageIn,
        {
            "argstr": "-template",
            "help_string": """define the output grid defined by a template image""",
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

transformcompose_input_spec = specs.SpecInfo(
    name="transformcompose_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = []
transformcompose_output_spec = specs.SpecInfo(
    name="transformcompose_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class transformcompose(ShellCommandTask):
    """Any linear transforms must be supplied as a 4x4 matrix in a text file (e.g. as per the output of mrregister). Any warp fields must be supplied as a 4D image representing a deformation field (e.g. as output from mrrregister -nl_warp).

        Input transformations should be provided to the command in the order in which they would be applied to an image if they were to be applied individually.

        If all input transformations are linear, and the -template option is not provided, then the file output by the command will also be a linear transformation saved as a 4x4 matrix in a text file. If a template image is supplied, then the output will always be a deformation field. If at least one of the inputs is a warp field, then the output will be a deformation field, which will be defined on the grid of the last input warp image supplied if the -template option is not used.


        References
        ----------

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: David Raffelt (david.raffelt@florey.edu.au)

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

    executable = "transformcompose"
    input_spec = transformcompose_input_spec
    output_spec = transformcompose_output_spec
