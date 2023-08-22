# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "in_tracks",
        Tracks,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the input track file""",
            "mandatory": True,
        },
    ),
    (
        "out_tracks",
        Tracks,
        {
            "argstr": "",
            "position": 1,
            "help_string": """the output resampled tracks""",
            "mandatory": True,
        },
    ),
    # Streamline resampling options Option Group
    (
        "upsample",
        int,
        {
            "argstr": "-upsample",
            "help_string": """increase the density of points along the length of each streamline by some factor (may improve mapping streamlines to ROIs, and/or visualisation)""",
        },
    ),
    (
        "downsample",
        int,
        {
            "argstr": "-downsample",
            "help_string": """increase the density of points along the length of each streamline by some factor (decreases required storage space)""",
        },
    ),
    (
        "step_size",
        float,
        {
            "argstr": "-step_size",
            "help_string": """re-sample the streamlines to a desired step size (in mm)""",
        },
    ),
    (
        "num_points",
        int,
        {
            "argstr": "-num_points",
            "help_string": """re-sample each streamline to a fixed number of points""",
        },
    ),
    (
        "endpoints",
        bool,
        {
            "argstr": "-endpoints",
            "help_string": """only output the two endpoints of each streamline""",
        },
    ),
    (
        "line",
        ty.Tuple[int, int, int],
        {
            "argstr": "-line",
            "help_string": """resample tracks at 'num' equidistant locations along a line between 'start' and 'end' (specified as comma-separated 3-vectors in scanner coordinates)""",
        },
    ),
    (
        "arc",
        ty.Tuple[int, int, int, int],
        {
            "argstr": "-arc",
            "help_string": """resample tracks at 'num' equidistant locations along a circular arc specified by points 'start', 'mid' and 'end' (specified as comma-separated 3-vectors in scanner coordinates)""",
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

tckresample_input_spec = specs.SpecInfo(
    name="tckresample_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = []
tckresample_output_spec = specs.SpecInfo(
    name="tckresample_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class tckresample(ShellCommandTask):
    """It is necessary to specify precisely ONE of the command-line options for controlling how this resampling takes place; this may be either increasing or decreasing the number of samples along each streamline, or may involve changing the positions of the samples according to some specified trajectory.

        Note that because the length of a streamline is calculated based on the sums of distances between adjacent vertices, resampling a streamline to a new set of vertices will typically change the quantified length of that streamline; the magnitude of the difference will typically depend on the discrepancy in the number of vertices, with less vertices leading to a shorter length (due to taking chordal lengths of curved trajectories).


        References
        ----------

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

    executable = "tckresample"
    input_spec = tckresample_input_spec
    output_spec = tckresample_output_spec
