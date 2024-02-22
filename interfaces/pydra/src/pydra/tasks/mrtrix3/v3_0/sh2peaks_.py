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
        ImageIn,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the input image of SH coefficients.""",
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
            "help_string": """the output image. Each volume corresponds to the x, y & z component of each peak direction vector in turn.""",
        },
    ),
    (
        "num",
        int,
        {
            "argstr": "-num",
            "help_string": """the number of peaks to extract (default: 3).""",
        },
    ),
    (
        "direction",
        specs.MultiInputObj[ty.Tuple[float, float]],
        {
            "argstr": "-direction",
            "help_string": """the direction of a peak to estimate. The algorithm will attempt to find the same number of peaks as have been specified using this option.""",
        },
    ),
    (
        "peaks",
        ImageIn,
        {
            "argstr": "-peaks",
            "help_string": """the program will try to find the peaks that most closely match those in the image provided.""",
        },
    ),
    (
        "threshold",
        float,
        {
            "argstr": "-threshold",
            "help_string": """only peak amplitudes greater than the threshold will be considered.""",
        },
    ),
    (
        "seeds",
        File,
        {
            "argstr": "-seeds",
            "help_string": """specify a set of directions from which to start the multiple restarts of the optimisation (by default, the built-in 60 direction set is used)""",
        },
    ),
    (
        "mask",
        ImageIn,
        {
            "argstr": "-mask",
            "help_string": """only perform computation within the specified binary brain mask image.""",
        },
    ),
    (
        "fast",
        bool,
        {
            "argstr": "-fast",
            "help_string": """use lookup table to compute associated Legendre polynomials (faster, but approximate).""",
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

sh2peaks_input_spec = specs.SpecInfo(
    name="sh2peaks_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "output",
        ImageOut,
        {
            "help_string": """the output image. Each volume corresponds to the x, y & z component of each peak direction vector in turn.""",
        },
    ),
]
sh2peaks_output_spec = specs.SpecInfo(
    name="sh2peaks_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class sh2peaks(ShellCommandTask):
    """Peaks of the spherical harmonic function in each voxel are located by commencing a Newton search along each of a set of pre-specified directions

        The spherical harmonic coefficients are stored according to the conventions described in the main documentation, which can be found at the following link:
    https://mrtrix.readthedocs.io/en/3.0.4/concepts/spherical_harmonics.html


        References
        ----------

            Jeurissen, B.; Leemans, A.; Tournier, J.-D.; Jones, D.K.; Sijbers, J. Investigating the prevalence of complex fiber configurations in white matter tissue with diffusion magnetic resonance imaging. Human Brain Mapping, 2013, 34(11), 2747-2766

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

    executable = "sh2peaks"
    input_spec = sh2peaks_input_spec
    output_spec = sh2peaks_output_spec
