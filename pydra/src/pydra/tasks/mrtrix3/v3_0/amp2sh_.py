# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "amp",
        ImageIn,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the input amplitude image.""",
            "mandatory": True,
        },
    ),
    (
        "SH",
        Path,
        {
            "argstr": "",
            "position": 1,
            "output_file_template": "SH.mif",
            "help_string": """the output spherical harmonics coefficients image.""",
        },
    ),
    (
        "lmax",
        int,
        {
            "argstr": "-lmax",
            "help_string": """set the maximum harmonic order for the output series. By default, the program will use the highest possible lmax given the number of diffusion-weighted images, up to a maximum of 8.""",
        },
    ),
    (
        "normalise",
        bool,
        {
            "argstr": "-normalise",
            "help_string": """normalise the DW signal to the b=0 image""",
        },
    ),
    (
        "directions",
        File,
        {
            "argstr": "-directions",
            "help_string": """the directions corresponding to the input amplitude image used to sample AFD. By default this option is not required providing the direction set is supplied in the amplitude image. This should be supplied as a list of directions [az el], as generated using the dirgen command, or as a list of [ x y z ] Cartesian coordinates.""",
        },
    ),
    (
        "rician",
        ImageIn,
        {
            "argstr": "-rician",
            "help_string": """correct for Rician noise induced bias, using noise map supplied""",
        },
    ),
    # DW gradient table import options Option Group
    (
        "grad",
        File,
        {
            "argstr": "-grad",
            "help_string": """Provide the diffusion-weighted gradient scheme used in the acquisition in a text file. This should be supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.""",
        },
    ),
    (
        "fslgrad",
        ty.Tuple[File, File],
        {
            "argstr": "-fslgrad",
            "help_string": """Provide the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format files. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.""",
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
    # Stride options Option Group
    (
        "strides",
        ty.Any,
        {
            "argstr": "-strides",
            "help_string": """specify the strides of the output data in memory; either as a comma-separated list of (signed) integers, or as a template image from which the strides shall be extracted and used. The actual strides produced will depend on whether the output image format can support it.""",
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

amp2sh_input_spec = specs.SpecInfo(
    name="amp2sh_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "SH",
        ImageOut,
        {
            "help_string": """the output spherical harmonics coefficients image.""",
        },
    ),
]
amp2sh_output_spec = specs.SpecInfo(
    name="amp2sh_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class amp2sh(ShellCommandTask):
    """The spherical harmonic decomposition is calculated by least-squares linear fitting to the amplitude data.

        The directions can be defined either as a DW gradient scheme (for example to compute the SH representation of the DW signal), a set of [az el] pairs as output by the dirgen command, or a set of [ x y z ] directions in Cartesian coordinates. The DW gradient scheme or direction set can be supplied within the input image header or using the -gradient or -directions option. Note that if a direction set and DW gradient scheme can be found, the direction set will be used by default.

        The spherical harmonic coefficients are stored according to the conventions described in the main documentation, which can be found at the following link:
    https://mrtrix.readthedocs.io/en/3.0.4/concepts/spherical_harmonics.html


        References
        ----------

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

    executable = "amp2sh"
    input_spec = amp2sh_input_spec
    output_spec = amp2sh_output_spec
