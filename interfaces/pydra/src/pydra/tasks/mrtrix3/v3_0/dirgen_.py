# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "ndir",
        int,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the number of directions to generate.""",
            "mandatory": True,
        },
    ),
    (
        "dirs",
        Path,
        {
            "argstr": "",
            "position": 1,
            "output_file_template": "dirs.txt",
            "help_string": """the text file to write the directions to, as [ az el ] pairs.""",
        },
    ),
    (
        "power",
        int,
        {
            "argstr": "-power",
            "help_string": """specify exponent to use for repulsion power law (default: 1). This must be a power of 2 (i.e. 1, 2, 4, 8, 16, ...).""",
        },
    ),
    (
        "niter",
        int,
        {
            "argstr": "-niter",
            "help_string": """specify the maximum number of iterations to perform (default: 10000).""",
        },
    ),
    (
        "restarts",
        int,
        {
            "argstr": "-restarts",
            "help_string": """specify the number of restarts to perform (default: 10).""",
        },
    ),
    (
        "unipolar",
        bool,
        {
            "argstr": "-unipolar",
            "help_string": """optimise assuming a unipolar electrostatic repulsion model rather than the bipolar model normally assumed in DWI""",
        },
    ),
    (
        "cartesian",
        bool,
        {
            "argstr": "-cartesian",
            "help_string": """Output the directions in Cartesian coordinates [x y z] instead of [az el].""",
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

dirgen_input_spec = specs.SpecInfo(
    name="dirgen_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "dirs",
        File,
        {
            "help_string": """the text file to write the directions to, as [ az el ] pairs.""",
        },
    ),
]
dirgen_output_spec = specs.SpecInfo(
    name="dirgen_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class dirgen(ShellCommandTask):
    """Directions are distributed by analogy to an electrostatic repulsion system, with each direction corresponding to a single electrostatic charge (for -unipolar), or a pair of diametrically opposed charges (for the default bipolar case). The energy of the system is determined based on the Coulomb repulsion, which assumes the form 1/r^power, where r is the distance between any pair of charges, and p is the power assumed for the repulsion law (default: 1). The minimum energy state is obtained by gradient descent.


        References
        ----------

            Jones, D.; Horsfield, M. & Simmons, A. Optimal strategies for measuring diffusion in anisotropic systems by magnetic resonance imaging. Magnetic Resonance in Medicine, 1999, 42: 515-525

            Papadakis, N. G.; Murrills, C. D.; Hall, L. D.; Huang, C. L.-H. & Adrian Carpenter, T. Minimal gradient encoding for robust estimation of diffusion anisotropy. Magnetic Resonance Imaging, 2000, 18: 671-679

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

    executable = "dirgen"
    input_spec = dirgen_input_spec
    output_spec = dirgen_output_spec
