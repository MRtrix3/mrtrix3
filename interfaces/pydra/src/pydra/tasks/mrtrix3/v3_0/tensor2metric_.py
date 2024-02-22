# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "tensor",
        ImageIn,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the input tensor image.""",
            "mandatory": True,
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
    # Diffusion Tensor Imaging Option Group
    (
        "adc",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-adc",
            "output_file_template": "adc.mif",
            "help_string": """compute the mean apparent diffusion coefficient (ADC) of the diffusion tensor. (sometimes also referred to as the mean diffusivity (MD))""",
        },
    ),
    (
        "fa",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-fa",
            "output_file_template": "fa.mif",
            "help_string": """compute the fractional anisotropy (FA) of the diffusion tensor.""",
        },
    ),
    (
        "ad",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-ad",
            "output_file_template": "ad.mif",
            "help_string": """compute the axial diffusivity (AD) of the diffusion tensor. (equivalent to the principal eigenvalue)""",
        },
    ),
    (
        "rd",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-rd",
            "output_file_template": "rd.mif",
            "help_string": """compute the radial diffusivity (RD) of the diffusion tensor. (equivalent to the mean of the two non-principal eigenvalues)""",
        },
    ),
    (
        "value",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-value",
            "output_file_template": "value.mif",
            "help_string": """compute the selected eigenvalue(s) of the diffusion tensor.""",
        },
    ),
    (
        "vector",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-vector",
            "output_file_template": "vector.mif",
            "help_string": """compute the selected eigenvector(s) of the diffusion tensor.""",
        },
    ),
    (
        "num",
        ty.List[int],
        {
            "argstr": "-num",
            "help_string": """specify the desired eigenvalue/eigenvector(s). Note that several eigenvalues can be specified as a number sequence. For example, '1,3' specifies the principal (1) and minor (3) eigenvalues/eigenvectors (default = 1).""",
            "sep": ",",
        },
    ),
    (
        "modulate",
        str,
        {
            "argstr": "-modulate",
            "help_string": """specify how to modulate the magnitude of the eigenvectors. Valid choices are: none, FA, eigval (default = FA).""",
            "allowed_values": ["none", "none", "fa", "eigval"],
        },
    ),
    (
        "cl",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-cl",
            "output_file_template": "cl.mif",
            "help_string": """compute the linearity metric of the diffusion tensor. (one of the three Westin shape metrics)""",
        },
    ),
    (
        "cp",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-cp",
            "output_file_template": "cp.mif",
            "help_string": """compute the planarity metric of the diffusion tensor. (one of the three Westin shape metrics)""",
        },
    ),
    (
        "cs",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-cs",
            "output_file_template": "cs.mif",
            "help_string": """compute the sphericity metric of the diffusion tensor. (one of the three Westin shape metrics)""",
        },
    ),
    # Diffusion Kurtosis Imaging Option Group
    (
        "dkt",
        ImageIn,
        {
            "argstr": "-dkt",
            "help_string": """input diffusion kurtosis tensor.""",
        },
    ),
    (
        "mk",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-mk",
            "output_file_template": "mk.mif",
            "help_string": """compute the mean kurtosis (MK) of the kurtosis tensor.""",
        },
    ),
    (
        "ak",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-ak",
            "output_file_template": "ak.mif",
            "help_string": """compute the axial kurtosis (AK) of the kurtosis tensor.""",
        },
    ),
    (
        "rk",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-rk",
            "output_file_template": "rk.mif",
            "help_string": """compute the radial kurtosis (RK) of the kurtosis tensor.""",
        },
    ),
    (
        "mk_dirs",
        File,
        {
            "argstr": "-mk_dirs",
            "help_string": """specify the directions used to numerically calculate mean kurtosis (by default, the built-in 300 direction set is used). These should be supplied as a text file containing [ az el ] pairs for the directions.""",
        },
    ),
    (
        "rk_ndirs",
        int,
        {
            "argstr": "-rk_ndirs",
            "help_string": """specify the number of directions used to numerically calculate radial kurtosis (by default, 300 directions are used).""",
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

tensor2metric_input_spec = specs.SpecInfo(
    name="tensor2metric_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "adc",
        ImageOut,
        {
            "help_string": """compute the mean apparent diffusion coefficient (ADC) of the diffusion tensor. (sometimes also referred to as the mean diffusivity (MD))""",
        },
    ),
    (
        "fa",
        ImageOut,
        {
            "help_string": """compute the fractional anisotropy (FA) of the diffusion tensor.""",
        },
    ),
    (
        "ad",
        ImageOut,
        {
            "help_string": """compute the axial diffusivity (AD) of the diffusion tensor. (equivalent to the principal eigenvalue)""",
        },
    ),
    (
        "rd",
        ImageOut,
        {
            "help_string": """compute the radial diffusivity (RD) of the diffusion tensor. (equivalent to the mean of the two non-principal eigenvalues)""",
        },
    ),
    (
        "value",
        ImageOut,
        {
            "help_string": """compute the selected eigenvalue(s) of the diffusion tensor.""",
        },
    ),
    (
        "vector",
        ImageOut,
        {
            "help_string": """compute the selected eigenvector(s) of the diffusion tensor.""",
        },
    ),
    (
        "cl",
        ImageOut,
        {
            "help_string": """compute the linearity metric of the diffusion tensor. (one of the three Westin shape metrics)""",
        },
    ),
    (
        "cp",
        ImageOut,
        {
            "help_string": """compute the planarity metric of the diffusion tensor. (one of the three Westin shape metrics)""",
        },
    ),
    (
        "cs",
        ImageOut,
        {
            "help_string": """compute the sphericity metric of the diffusion tensor. (one of the three Westin shape metrics)""",
        },
    ),
    (
        "mk",
        ImageOut,
        {
            "help_string": """compute the mean kurtosis (MK) of the kurtosis tensor.""",
        },
    ),
    (
        "ak",
        ImageOut,
        {
            "help_string": """compute the axial kurtosis (AK) of the kurtosis tensor.""",
        },
    ),
    (
        "rk",
        ImageOut,
        {
            "help_string": """compute the radial kurtosis (RK) of the kurtosis tensor.""",
        },
    ),
]
tensor2metric_output_spec = specs.SpecInfo(
    name="tensor2metric_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class tensor2metric(ShellCommandTask):
    """
        References
        ----------

            Basser, P. J.; Mattiello, J. & Lebihan, D. MR diffusion tensor spectroscopy and imaging. Biophysical Journal, 1994, 66, 259-267

            Westin, C. F.; Peled, S.; Gudbjartsson, H.; Kikinis, R. & Jolesz, F. A. Geometrical diffusion measures for MRI from tensor basis analysis. Proc Intl Soc Mag Reson Med, 1997, 5, 1742

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: Ben Jeurissen (ben.jeurissen@uantwerpen.be), Thijs Dhollander (thijs.dhollander@gmail.com) & J-Donald Tournier (jdtournier@gmail.com)

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

    executable = "tensor2metric"
    input_spec = tensor2metric_input_spec
    output_spec = tensor2metric_output_spec
