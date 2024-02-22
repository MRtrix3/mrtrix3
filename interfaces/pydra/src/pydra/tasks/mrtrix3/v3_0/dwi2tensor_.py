# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "dwi",
        ImageIn,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the input dwi image.""",
            "mandatory": True,
        },
    ),
    (
        "dt",
        Path,
        {
            "argstr": "",
            "position": 1,
            "output_file_template": "dt.mif",
            "help_string": """the output dt image.""",
        },
    ),
    (
        "ols",
        bool,
        {
            "argstr": "-ols",
            "help_string": """perform initial fit using an ordinary least-squares (OLS) fit (see Description).""",
        },
    ),
    (
        "iter",
        int,
        {
            "argstr": "-iter",
            "help_string": """number of iterative reweightings for IWLS algorithm (default: 2) (see Description).""",
        },
    ),
    (
        "constrain",
        bool,
        {
            "argstr": "-constrain",
            "help_string": """constrain fit to non-negative diffusivity and kurtosis as well as monotonic signal decay (see Description).""",
        },
    ),
    (
        "directions",
        File,
        {
            "argstr": "-directions",
            "help_string": """specify the directions along which to apply the constraints (by default, the built-in 300 direction set is used). These should be supplied as a text file containing [ az el ] pairs for the directions.""",
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
        "b0",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-b0",
            "output_file_template": "b0.mif",
            "help_string": """the output b0 image.""",
        },
    ),
    (
        "dkt",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-dkt",
            "output_file_template": "dkt.mif",
            "help_string": """the output dkt image.""",
        },
    ),
    (
        "predicted_signal",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-predicted_signal",
            "output_file_template": "predicted_signal.mif",
            "help_string": """the predicted dwi image.""",
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

dwi2tensor_input_spec = specs.SpecInfo(
    name="dwi2tensor_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "dt",
        ImageOut,
        {
            "help_string": """the output dt image.""",
        },
    ),
    (
        "b0",
        ImageOut,
        {
            "help_string": """the output b0 image.""",
        },
    ),
    (
        "dkt",
        ImageOut,
        {
            "help_string": """the output dkt image.""",
        },
    ),
    (
        "predicted_signal",
        ImageOut,
        {
            "help_string": """the predicted dwi image.""",
        },
    ),
]
dwi2tensor_output_spec = specs.SpecInfo(
    name="dwi2tensor_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class dwi2tensor(ShellCommandTask):
    """By default, the diffusion tensor (and optionally the kurtosis tensor) is fitted to the log-signal in two steps: firstly, using weighted least-squares (WLS) with weights based on the empirical signal intensities; secondly, by further iterated weighted least-squares (IWLS) with weights determined by the signal predictions from the previous iteration (by default, 2 iterations will be performed). This behaviour can be altered in two ways:

        * The -ols option will cause the first fitting step to be performed using ordinary least-squares (OLS); that is, all measurements contribute equally to the fit, instead of the default behaviour of weighting based on the empirical signal intensities.

        * The -iter option controls the number of iterations of the IWLS prodedure. If this is set to zero, then the output model parameters will be those resulting from the first fitting step only: either WLS by default, or OLS if the -ols option is used in conjunction with -iter 0.

        By default, the diffusion tensor (and optionally the kurtosis tensor) is fitted using unconstrained optimization. This can result in unexpected diffusion parameters, e.g. parameters that represent negative apparent diffusivities or negative apparent kurtoses, or parameters that correspond to non-monotonic decay of the predicted signal. By supplying the -constrain option, constrained optimization is performed instead and such physically implausible parameters can be avoided. Depending on the presence  of the -dkt option, the -constrain option will enforce the following constraints:

        * Non-negative apparent diffusivity (always).

        * Non-negative apparent kurtosis (when the -dkt option is provided).

        * Monotonic signal decay in the b = [0 b_max] range (when the -dkt option is provided).

        The tensor coefficients are stored in the output image as follows:
    volumes 0-5: D11, D22, D33, D12, D13, D23

        If diffusion kurtosis is estimated using the -dkt option, these are stored as follows:
    volumes 0-2: W1111, W2222, W3333
    volumes 3-8: W1112, W1113, W1222, W1333, W2223, W2333
    volumes 9-11: W1122, W1133, W2233
    volumes 12-14: W1123, W1223, W1233


        References
        ----------

            References based on fitting algorithm used:

            * OLS, WLS:
    Basser, P.J.; Mattiello, J.; LeBihan, D. Estimation of the effective self-diffusion tensor from the NMR spin echo. J Magn Reson B., 1994, 103, 247–254.

            * IWLS:
    Veraart, J.; Sijbers, J.; Sunaert, S.; Leemans, A. & Jeurissen, B. Weighted linear least squares estimation of diffusion MRI parameters: strengths, limitations, and pitfalls. NeuroImage, 2013, 81, 335-346

            * any of above with constraints:
    Morez, J.; Szczepankiewicz, F; den Dekker, A. J.; Vanhevel, F.; Sijbers, J. &  Jeurissen, B. Optimal experimental design and estimation for q-space trajectory imaging. Human Brain Mapping, In press

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: Ben Jeurissen (ben.jeurissen@uantwerpen.be)

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

    executable = "dwi2tensor"
    input_spec = dwi2tensor_input_spec
    output_spec = dwi2tensor_output_spec
