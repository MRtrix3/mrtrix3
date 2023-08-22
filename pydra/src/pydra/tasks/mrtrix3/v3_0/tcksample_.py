# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "tracks",
        Tracks,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the input track file""",
            "mandatory": True,
        },
    ),
    (
        "image_",
        ImageIn,
        {
            "argstr": "",
            "position": 1,
            "help_string": """the image to be sampled""",
            "mandatory": True,
        },
    ),
    (
        "values",
        Path,
        {
            "argstr": "",
            "position": 2,
            "output_file_template": "values.txt",
            "help_string": """the output sampled values""",
        },
    ),
    (
        "stat_tck",
        str,
        {
            "argstr": "-stat_tck",
            "help_string": """compute some statistic from the values along each streamline (options are: mean,median,min,max)""",
            "allowed_values": ["mean", "mean", "median", "min", "max"],
        },
    ),
    (
        "nointerp",
        bool,
        {
            "argstr": "-nointerp",
            "help_string": """do not use trilinear interpolation when sampling image values""",
        },
    ),
    (
        "precise",
        bool,
        {
            "argstr": "-precise",
            "help_string": """use the precise mechanism for mapping streamlines to voxels (obviates the need for trilinear interpolation) (only applicable if some per-streamline statistic is requested)""",
        },
    ),
    (
        "use_tdi_fraction",
        bool,
        {
            "argstr": "-use_tdi_fraction",
            "help_string": """each streamline is assigned a fraction of the image intensity in each voxel based on the fraction of the track density contributed by that streamline (this is only appropriate for processing a whole-brain tractogram, and images for which the quantiative parameter is additive)""",
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

tcksample_input_spec = specs.SpecInfo(
    name="tcksample_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "values",
        File,
        {
            "help_string": """the output sampled values""",
        },
    ),
]
tcksample_output_spec = specs.SpecInfo(
    name="tcksample_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class tcksample(ShellCommandTask):
    """By default, the value of the underlying image at each point along the track is written to either an ASCII file (with all values for each track on the same line), or a track scalar file (.tsf). Alternatively, some statistic can be taken from the values along each streamline and written to a vector file.


        References
        ----------

            * If using -precise option: Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. SIFT: Spherical-deconvolution informed filtering of tractograms. NeuroImage, 2013, 67, 298-312

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

    executable = "tcksample"
    input_spec = tcksample_input_spec
    output_spec = tcksample_output_spec
