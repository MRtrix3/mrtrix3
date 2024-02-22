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
        File,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the input mesh file""",
            "mandatory": True,
        },
    ),
    (
        "filter",
        str,
        {
            "argstr": "",
            "position": 1,
            "help_string": """the filter to apply.Options are: smooth""",
            "mandatory": True,
            "allowed_values": ["smooth", "smooth"],
        },
    ),
    (
        "output",
        Path,
        {
            "argstr": "",
            "position": 2,
            "output_file_template": "output.txt",
            "help_string": """the output mesh file""",
        },
    ),
    # Options for mesh smoothing filter Option Group
    (
        "smooth_spatial",
        float,
        {
            "argstr": "-smooth_spatial",
            "help_string": """spatial extent of smoothing (default: 10mm)""",
        },
    ),
    (
        "smooth_influence",
        float,
        {
            "argstr": "-smooth_influence",
            "help_string": """influence factor for smoothing (default: 10)""",
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

meshfilter_input_spec = specs.SpecInfo(
    name="meshfilter_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "output",
        File,
        {
            "help_string": """the output mesh file""",
        },
    ),
]
meshfilter_output_spec = specs.SpecInfo(
    name="meshfilter_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class meshfilter(ShellCommandTask):
    """While this command has only one filter operation currently available, it nevertheless presents with a comparable interface to the MRtrix3 commands maskfilter and mrfilter commands.


        Example usages
        --------------


        Apply a mesh smoothing filter (currently the only filter available:

            `$ meshfilter input.vtk smooth output.vtk`

            The usage of this command may cause confusion due to the generic interface despite only one filtering operation being currently available. This simple example usage is therefore provided for clarity.


        References
        ----------

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

    executable = "meshfilter"
    input_spec = meshfilter_input_spec
    output_spec = meshfilter_output_spec
