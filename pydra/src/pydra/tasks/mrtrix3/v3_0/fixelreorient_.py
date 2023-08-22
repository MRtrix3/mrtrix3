# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "fixel_in",
        Directory,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the input fixel directory""",
            "mandatory": True,
        },
    ),
    (
        "warp",
        ImageIn,
        {
            "argstr": "",
            "position": 1,
            "help_string": """a 4D deformation field used to perform reorientation. Reorientation is performed by applying the Jacobian affine transform in each voxel in the warp, then re-normalising the vector representing the fixel direction""",
            "mandatory": True,
        },
    ),
    (
        "fixel_out",
        Path,
        {
            "argstr": "",
            "position": 2,
            "output_file_template": "fixel_out",
            "help_string": """the output fixel directory. If the the input and output directories are the same, the existing directions file will be replaced (providing the -force option is supplied). If a new directory is supplied then the fixel directions and all other fixel data will be copied to the new directory.""",
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

fixelreorient_input_spec = specs.SpecInfo(
    name="fixelreorient_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "fixel_out",
        Directory,
        {
            "help_string": """the output fixel directory. If the the input and output directories are the same, the existing directions file will be replaced (providing the -force option is supplied). If a new directory is supplied then the fixel directions and all other fixel data will be copied to the new directory.""",
        },
    ),
]
fixelreorient_output_spec = specs.SpecInfo(
    name="fixelreorient_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class fixelreorient(ShellCommandTask):
    """Reorientation is performed by transforming the vector representing the fixel direction with the Jacobian (local affine transform) computed at each voxel in the warp, then re-normalising the vector.

        Fixel data are stored utilising the fixel directory format described in the main documentation, which can be found at the following link:
    https://mrtrix.readthedocs.io/en/3.0.4/fixel_based_analysis/fixel_directory_format.html


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

    executable = "fixelreorient"
    input_spec = fixelreorient_input_spec
    output_spec = fixelreorient_output_spec
