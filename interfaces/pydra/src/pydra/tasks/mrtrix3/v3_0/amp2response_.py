# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "amps",
        ImageIn,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the amplitudes image""",
            "mandatory": True,
        },
    ),
    (
        "mask",
        ImageIn,
        {
            "argstr": "",
            "position": 1,
            "help_string": """the mask containing the voxels from which to estimate the response function""",
            "mandatory": True,
        },
    ),
    (
        "directions_image",
        ImageIn,
        {
            "argstr": "",
            "position": 2,
            "help_string": """a 4D image containing the estimated fibre directions""",
            "mandatory": True,
        },
    ),
    (
        "response",
        Path,
        {
            "argstr": "",
            "position": 3,
            "output_file_template": "response.txt",
            "help_string": """the output zonal spherical harmonic coefficients""",
        },
    ),
    (
        "isotropic",
        bool,
        {
            "argstr": "-isotropic",
            "help_string": """estimate an isotropic response function (lmax=0 for all shells)""",
        },
    ),
    (
        "noconstraint",
        bool,
        {
            "argstr": "-noconstraint",
            "help_string": """disable the non-negativity and monotonicity constraints""",
        },
    ),
    (
        "directions",
        File,
        {
            "argstr": "-directions",
            "help_string": """provide an external text file containing the directions along which the amplitudes are sampled""",
        },
    ),
    # DW shell selection options Option Group
    (
        "shells",
        ty.List[float],
        {
            "argstr": "-shells",
            "help_string": """specify one or more b-values to use during processing, as a comma-separated list of the desired approximate b-values (b-values are clustered to allow for small deviations). Note that some commands are incompatible with multiple b-values, and will report an error if more than one b-value is provided. 
WARNING: note that, even though the b=0 volumes are never referred to as shells in the literature, they still have to be explicitly included in the list of b-values as provided to the -shell option! Several algorithms which include the b=0 volumes in their computations may otherwise return an undesired result.""",
            "sep": ",",
        },
    ),
    (
        "lmax",
        ty.List[int],
        {
            "argstr": "-lmax",
            "help_string": """specify the maximum harmonic degree of the response function to estimate (can be a comma-separated list for multi-shell data)""",
            "sep": ",",
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

amp2response_input_spec = specs.SpecInfo(
    name="amp2response_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "response",
        File,
        {
            "help_string": """the output zonal spherical harmonic coefficients""",
        },
    ),
]
amp2response_output_spec = specs.SpecInfo(
    name="amp2response_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class amp2response(ShellCommandTask):
    """This command uses the image data from all selected single-fibre voxels concurrently, rather than simply averaging their individual spherical harmonic coefficients. It also ensures that the response function is non-negative, and monotonic (i.e. its amplitude must increase from the fibre direction out to the orthogonal plane).

        If multi-shell data are provided, and one or more b-value shells are not explicitly requested, the command will generate a response function for every b-value shell (including b=0 if present).


        References
        ----------

            Smith, R. E.; Dhollander, T. & Connelly, A. Constrained linear least squares estimation of anisotropic response function for spherical deconvolution. ISMRM Workshop on Breaking the Barriers of Diffusion MRI, 23.

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

    executable = "amp2response"
    input_spec = amp2response_input_spec
    output_spec = amp2response_output_spec
