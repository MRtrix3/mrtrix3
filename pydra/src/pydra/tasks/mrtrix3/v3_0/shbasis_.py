# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "SH",
        specs.MultiInputObj[ImageIn],
        {
            "argstr": "",
            "position": 0,
            "help_string": """the input image(s) of SH coefficients.""",
            "mandatory": True,
        },
    ),
    (
        "convert",
        str,
        {
            "argstr": "-convert",
            "help_string": """convert the image data in-place to the desired basis; options are: old,new,force_oldtonew,force_newtoold.""",
            "allowed_values": ["old", "old", "new", "force_oldtonew", "force_newtoold"],
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

shbasis_input_spec = specs.SpecInfo(
    name="shbasis_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = []
shbasis_output_spec = specs.SpecInfo(
    name="shbasis_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class shbasis(ShellCommandTask):
    """In previous versions of MRtrix, the convention used for storing spherical harmonic coefficients was a non-orthonormal basis (the m!=0 coefficients were a factor of sqrt(2) too large). This error has been rectified in newer versions of MRtrix, but will cause issues if processing SH data that was generated using an older version of MRtrix (or vice-versa).

        This command provides a mechanism for testing the basis used in storage of image data representing a spherical harmonic series per voxel, and allows the user to forcibly modify the raw image data to conform to the desired basis.

        Note that the "force_*" conversion choices should only be used in cases where this command has previously been unable to automatically determine the SH basis from the image data, but the user themselves are confident of the SH basis of the data.

        The spherical harmonic coefficients are stored according to the conventions described in the main documentation, which can be found at the following link:
    https://mrtrix.readthedocs.io/en/3.0.4/concepts/spherical_harmonics.html


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

    executable = "shbasis"
    input_spec = shbasis_input_spec
    output_spec = shbasis_output_spec
