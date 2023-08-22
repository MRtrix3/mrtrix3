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
        "in_fod",
        ImageIn,
        {
            "argstr": "",
            "position": 1,
            "help_string": """input image containing the spherical harmonics of the fibre orientation distributions""",
            "mandatory": True,
        },
    ),
    (
        "out_weights",
        Path,
        {
            "argstr": "",
            "position": 2,
            "output_file_template": "out_weights.txt",
            "help_string": """output text file containing the weighting factor for each streamline""",
        },
    ),
    # Options for setting the processing mask for the SIFT fixel-streamlines comparison model Option Group
    (
        "proc_mask",
        ImageIn,
        {
            "argstr": "-proc_mask",
            "help_string": """provide an image containing the processing mask weights for the model; image spatial dimensions must match the fixel image""",
        },
    ),
    (
        "act",
        ImageIn,
        {
            "argstr": "-act",
            "help_string": """use an ACT five-tissue-type segmented anatomical image to derive the processing mask""",
        },
    ),
    # Options affecting the SIFT model Option Group
    (
        "fd_scale_gm",
        bool,
        {
            "argstr": "-fd_scale_gm",
            "help_string": """provide this option (in conjunction with -act) to heuristically downsize the fibre density estimates based on the presence of GM in the voxel. This can assist in reducing tissue interface effects when using a single-tissue deconvolution algorithm""",
        },
    ),
    (
        "no_dilate_lut",
        bool,
        {
            "argstr": "-no_dilate_lut",
            "help_string": """do NOT dilate FOD lobe lookup tables; only map streamlines to FOD lobes if the precise tangent lies within the angular spread of that lobe""",
        },
    ),
    (
        "make_null_lobes",
        bool,
        {
            "argstr": "-make_null_lobes",
            "help_string": """add an additional FOD lobe to each voxel, with zero integral, that covers all directions with zero / negative FOD amplitudes""",
        },
    ),
    (
        "remove_untracked",
        bool,
        {
            "argstr": "-remove_untracked",
            "help_string": """remove FOD lobes that do not have any streamline density attributed to them; this improves filtering slightly, at the expense of longer computation time (and you can no longer do quantitative comparisons between reconstructions if this is enabled)""",
        },
    ),
    (
        "fd_thresh",
        float,
        {
            "argstr": "-fd_thresh",
            "help_string": """fibre density threshold; exclude an FOD lobe from filtering processing if its integral is less than this amount (streamlines will still be mapped to it, but it will not contribute to the cost function or the filtering)""",
        },
    ),
    # Options to make SIFT provide additional output files Option Group
    (
        "csv",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-csv",
            "output_file_template": "csv.txt",
            "help_string": """output statistics of execution per iteration to a .csv file""",
        },
    ),
    (
        "out_mu",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-out_mu",
            "output_file_template": "out_mu.txt",
            "help_string": """output the final value of SIFT proportionality coefficient mu to a text file""",
        },
    ),
    (
        "output_debug",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-output_debug",
            "output_file_template": "output_debug",
            "help_string": """write to a directory various output images for assessing & debugging performance etc.""",
        },
    ),
    (
        "out_coeffs",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-out_coeffs",
            "output_file_template": "out_coeffs.txt",
            "help_string": """output text file containing the weighting coefficient for each streamline""",
        },
    ),
    # Regularisation options for SIFT2 Option Group
    (
        "reg_tikhonov",
        float,
        {
            "argstr": "-reg_tikhonov",
            "help_string": """provide coefficient for regularising streamline weighting coefficients (Tikhonov regularisation) (default: 0)""",
        },
    ),
    (
        "reg_tv",
        float,
        {
            "argstr": "-reg_tv",
            "help_string": """provide coefficient for regularising variance of streamline weighting coefficient to fixels along its length (Total Variation regularisation) (default: 0.1)""",
        },
    ),
    # Options for controlling the SIFT2 optimisation algorithm Option Group
    (
        "min_td_frac",
        float,
        {
            "argstr": "-min_td_frac",
            "help_string": """minimum fraction of the FOD integral reconstructed by streamlines; if the reconstructed streamline density is below this fraction, the fixel is excluded from optimisation (default: 0.1)""",
        },
    ),
    (
        "min_iters",
        int,
        {
            "argstr": "-min_iters",
            "help_string": """minimum number of iterations to run before testing for convergence; this can prevent premature termination at early iterations if the cost function increases slightly (default: 10)""",
        },
    ),
    (
        "max_iters",
        int,
        {
            "argstr": "-max_iters",
            "help_string": """maximum number of iterations to run before terminating program""",
        },
    ),
    (
        "min_factor",
        float,
        {
            "argstr": "-min_factor",
            "help_string": """minimum weighting factor for an individual streamline; if the factor falls below this number the streamline will be rejected entirely (factor set to zero) (default: 0)""",
        },
    ),
    (
        "min_coeff",
        float,
        {
            "argstr": "-min_coeff",
            "help_string": """minimum weighting coefficient for an individual streamline; similar to the '-min_factor' option, but using the exponential coefficient basis of the SIFT2 model; these parameters are related as: factor = e^(coeff). Note that the -min_factor and -min_coeff options are mutually exclusive - you can only provide one. (default: -inf)""",
        },
    ),
    (
        "max_factor",
        float,
        {
            "argstr": "-max_factor",
            "help_string": """maximum weighting factor that can be assigned to any one streamline (default: inf)""",
        },
    ),
    (
        "max_coeff",
        float,
        {
            "argstr": "-max_coeff",
            "help_string": """maximum weighting coefficient for an individual streamline; similar to the '-max_factor' option, but using the exponential coefficient basis of the SIFT2 model; these parameters are related as: factor = e^(coeff). Note that the -max_factor and -max_coeff options are mutually exclusive - you can only provide one. (default: inf)""",
        },
    ),
    (
        "max_coeff_step",
        float,
        {
            "argstr": "-max_coeff_step",
            "help_string": """maximum change to a streamline's weighting coefficient in a single iteration (default: 1)""",
        },
    ),
    (
        "min_cf_decrease",
        float,
        {
            "argstr": "-min_cf_decrease",
            "help_string": """minimum decrease in the cost function (as a fraction of the initial value) that must occur each iteration for the algorithm to continue (default: 2.5e-05)""",
        },
    ),
    (
        "linear",
        bool,
        {
            "argstr": "-linear",
            "help_string": """perform a linear estimation of streamline weights, rather than the standard non-linear optimisation (typically does not provide as accurate a model fit; but only requires a single pass)""",
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

tcksift2_input_spec = specs.SpecInfo(
    name="tcksift2_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "out_weights",
        File,
        {
            "help_string": """output text file containing the weighting factor for each streamline""",
        },
    ),
    (
        "csv",
        File,
        {
            "help_string": """output statistics of execution per iteration to a .csv file""",
        },
    ),
    (
        "out_mu",
        File,
        {
            "help_string": """output the final value of SIFT proportionality coefficient mu to a text file""",
        },
    ),
    (
        "output_debug",
        Directory,
        {
            "help_string": """write to a directory various output images for assessing & debugging performance etc.""",
        },
    ),
    (
        "out_coeffs",
        File,
        {
            "help_string": """output text file containing the weighting coefficient for each streamline""",
        },
    ),
]
tcksift2_output_spec = specs.SpecInfo(
    name="tcksift2_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class tcksift2(ShellCommandTask):
    """
        References
        ----------

            Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. SIFT2: Enabling dense quantitative assessment of brain white matter connectivity using streamlines tractography. NeuroImage, 2015, 119, 338-351

            * If using the -linear option:
    Smith, RE; Raffelt, D; Tournier, J-D; Connelly, A. Quantitative Streamlines Tractography: Methods and Inter-Subject Normalisation. Open Science Framework, https://doi.org/10.31219/osf.io/c67kn.

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

    executable = "tcksift2"
    input_spec = tcksift2_input_spec
    output_spec = tcksift2_output_spec
