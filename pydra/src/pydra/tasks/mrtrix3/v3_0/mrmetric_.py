# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "image1",
        ImageIn,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the first input image.""",
            "mandatory": True,
        },
    ),
    (
        "image2",
        ImageIn,
        {
            "argstr": "",
            "position": 1,
            "help_string": """the second input image.""",
            "mandatory": True,
        },
    ),
    (
        "space",
        str,
        {
            "argstr": "-space",
            "help_string": """voxel (default): per voxel image1: scanner space of image 1 image2: scanner space of image 2 average: scanner space of the average affine transformation of image 1 and 2 """,
            "allowed_values": ["voxel", "voxel", "image1", "image2", "average"],
        },
    ),
    (
        "interp",
        str,
        {
            "argstr": "-interp",
            "help_string": """set the interpolation method to use when reslicing (choices: nearest, linear, cubic, sinc. Default: linear).""",
            "allowed_values": ["nearest", "nearest", "linear", "cubic", "sinc"],
        },
    ),
    (
        "metric",
        str,
        {
            "argstr": "-metric",
            "help_string": """define the dissimilarity metric used to calculate the cost. Choices: diff (squared differences), cc (non-normalised negative cross correlation aka negative cross covariance). Default: diff). cc is only implemented for -space average and -interp linear and cubic.""",
            "allowed_values": ["diff", "diff", "cc"],
        },
    ),
    (
        "mask1",
        ImageIn,
        {
            "argstr": "-mask1",
            "help_string": """mask for image 1""",
        },
    ),
    (
        "mask2",
        ImageIn,
        {
            "argstr": "-mask2",
            "help_string": """mask for image 2""",
        },
    ),
    (
        "nonormalisation",
        bool,
        {
            "argstr": "-nonormalisation",
            "help_string": """do not normalise the dissimilarity metric to the number of voxels.""",
        },
    ),
    (
        "overlap",
        bool,
        {
            "argstr": "-overlap",
            "help_string": """output number of voxels that were used.""",
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

mrmetric_input_spec = specs.SpecInfo(
    name="mrmetric_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = []
mrmetric_output_spec = specs.SpecInfo(
    name="mrmetric_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class mrmetric(ShellCommandTask):
    """Currently only the mean squared difference is fully implemented.


        References
        ----------

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: David Raffelt (david.raffelt@florey.edu.au) and Max Pietsch (maximilian.pietsch@kcl.ac.uk)

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

    executable = "mrmetric"
    input_spec = mrmetric_input_spec
    output_spec = mrmetric_output_spec
