# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "in_fixel_directory",
        Directory,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the fixel directory containing the data files for each subject (after obtaining fixel correspondence""",
            "mandatory": True,
        },
    ),
    (
        "subjects",
        ImageIn,
        {
            "argstr": "",
            "position": 1,
            "help_string": """a text file listing the subject identifiers (one per line). This should correspond with the filenames in the fixel directory (including the file extension), and be listed in the same order as the rows of the design matrix.""",
            "mandatory": True,
        },
    ),
    (
        "design",
        File,
        {
            "argstr": "",
            "position": 2,
            "help_string": """the design matrix""",
            "mandatory": True,
        },
    ),
    (
        "contrast",
        File,
        {
            "argstr": "",
            "position": 3,
            "help_string": """the contrast matrix, specified as rows of weights""",
            "mandatory": True,
        },
    ),
    (
        "connectivity",
        ty.Any,
        {
            "argstr": "",
            "position": 4,
            "help_string": """the fixel-fixel connectivity matrix""",
            "mandatory": True,
        },
    ),
    (
        "out_fixel_directory",
        str,
        {
            "argstr": "",
            "position": 5,
            "help_string": """the output directory where results will be saved. Will be created if it does not exist""",
            "mandatory": True,
        },
    ),
    (
        "mask",
        ImageIn,
        {
            "argstr": "-mask",
            "help_string": """provide a fixel data file containing a mask of those fixels to be used during processing""",
        },
    ),
    # Options relating to shuffling of data for nonparametric statistical inference Option Group
    (
        "notest",
        bool,
        {
            "argstr": "-notest",
            "help_string": """don't perform statistical inference; only output population statistics (effect size, stdev etc)""",
        },
    ),
    (
        "errors",
        str,
        {
            "argstr": "-errors",
            "help_string": """specify nature of errors for shuffling; options are: ee,ise,both (default: ee)""",
            "allowed_values": ["ee", "ee", "ise", "both"],
        },
    ),
    (
        "exchange_within",
        File,
        {
            "argstr": "-exchange_within",
            "help_string": """specify blocks of observations within each of which data may undergo restricted exchange""",
        },
    ),
    (
        "exchange_whole",
        File,
        {
            "argstr": "-exchange_whole",
            "help_string": """specify blocks of observations that may be exchanged with one another (for independent and symmetric errors, sign-flipping will occur block-wise)""",
        },
    ),
    (
        "strong",
        bool,
        {
            "argstr": "-strong",
            "help_string": """use strong familywise error control across multiple hypotheses""",
        },
    ),
    (
        "nshuffles",
        int,
        {
            "argstr": "-nshuffles",
            "help_string": """the number of shuffles (default: 5000)""",
        },
    ),
    (
        "permutations",
        File,
        {
            "argstr": "-permutations",
            "help_string": """manually define the permutations (relabelling). The input should be a text file defining a m x n matrix, where each relabelling is defined as a column vector of size m, and the number of columns, n, defines the number of permutations. Can be generated with the palm_quickperms function in PALM (http://fsl.fmrib.ox.ac.uk/fsl/fslwiki/PALM). Overrides the -nshuffles option.""",
        },
    ),
    (
        "nonstationarity",
        bool,
        {
            "argstr": "-nonstationarity",
            "help_string": """perform non-stationarity correction""",
        },
    ),
    (
        "skew_nonstationarity",
        float,
        {
            "argstr": "-skew_nonstationarity",
            "help_string": """specify the skew parameter for empirical statistic calculation (default for this command is 1)""",
        },
    ),
    (
        "nshuffles_nonstationarity",
        int,
        {
            "argstr": "-nshuffles_nonstationarity",
            "help_string": """the number of shuffles to use when precomputing the empirical statistic image for non-stationarity correction (default: 5000)""",
        },
    ),
    (
        "permutations_nonstationarity",
        File,
        {
            "argstr": "-permutations_nonstationarity",
            "help_string": """manually define the permutations (relabelling) for computing the emprical statistics for non-stationarity correction. The input should be a text file defining a m x n matrix, where each relabelling is defined as a column vector of size m, and the number of columns, n, defines the number of permutations. Can be generated with the palm_quickperms function in PALM (http://fsl.fmrib.ox.ac.uk/fsl/fslwiki/PALM) Overrides the -nshuffles_nonstationarity option.""",
        },
    ),
    # Parameters for the Connectivity-based Fixel Enhancement algorithm Option Group
    (
        "cfe_dh",
        float,
        {
            "argstr": "-cfe_dh",
            "help_string": """the height increment used in the cfe integration (default: 0.1)""",
        },
    ),
    (
        "cfe_e",
        float,
        {
            "argstr": "-cfe_e",
            "help_string": """cfe extent exponent (default: 2)""",
        },
    ),
    (
        "cfe_h",
        float,
        {
            "argstr": "-cfe_h",
            "help_string": """cfe height exponent (default: 3)""",
        },
    ),
    (
        "cfe_c",
        float,
        {
            "argstr": "-cfe_c",
            "help_string": """cfe connectivity exponent (default: 0.5)""",
        },
    ),
    (
        "cfe_legacy",
        bool,
        {
            "argstr": "-cfe_legacy",
            "help_string": """use the legacy (non-normalised) form of the cfe equation""",
        },
    ),
    # Options related to the General Linear Model (GLM) Option Group
    (
        "variance",
        File,
        {
            "argstr": "-variance",
            "help_string": """define variance groups for the G-statistic; measurements for which the expected variance is equivalent should contain the same index""",
        },
    ),
    (
        "ftests",
        File,
        {
            "argstr": "-ftests",
            "help_string": """perform F-tests; input text file should contain, for each F-test, a row containing ones and zeros, where ones indicate the rows of the contrast matrix to be included in the F-test.""",
        },
    ),
    (
        "fonly",
        bool,
        {
            "argstr": "-fonly",
            "help_string": """only assess F-tests; do not perform statistical inference on entries in the contrast matrix""",
        },
    ),
    (
        "column",
        specs.MultiInputObj[File],
        {
            "argstr": "-column",
            "help_string": """add a column to the design matrix corresponding to subject fixel-wise values (note that the contrast matrix must include an additional column for each use of this option); the text file provided via this option should contain a file name for each subject""",
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

fixelcfestats_input_spec = specs.SpecInfo(
    name="fixelcfestats_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = []
fixelcfestats_output_spec = specs.SpecInfo(
    name="fixelcfestats_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class fixelcfestats(ShellCommandTask):
    """Unlike previous versions of this command, where a whole-brain tractogram file would be provided as input in order to generate the fixel-fixel connectivity matrix and smooth fixel data, this version expects to be provided with the directory path to a pre-calculated fixel-fixel connectivity matrix (likely generated using the MRtrix3 command fixelconnectivity), and for the input fixel data to have already been smoothed (likely using the MRtrix3 command fixelfilter).

        Note that if the -mask option is used, the output fixel directory will still contain the same set of fixels as that present in the input fixel template, in order to retain fixel correspondence. However a consequence of this is that all fixels in the template will be initialy visible when the output fixel directory is loaded in mrview. Those fixels outside the processing mask will immediately disappear from view as soon as any data-file-based fixel colouring or thresholding is applied.

        In some software packages, a column of ones is automatically added to the GLM design matrix; the purpose of this column is to estimate the "global intercept", which is the predicted value of the observed variable if all explanatory variables were to be zero. However there are rare situations where including such a column would not be appropriate for a particular experimental design. Hence, in MRtrix3 statistical inference commands, it is up to the user to determine whether or not this column of ones should be included in their design matrix, and add it explicitly if necessary. The contrast matrix must also reflect the presence of this additional column.

        Fixel data are stored utilising the fixel directory format described in the main documentation, which can be found at the following link:
    https://mrtrix.readthedocs.io/en/3.0.4/fixel_based_analysis/fixel_directory_format.html


        References
        ----------

            Raffelt, D.; Smith, RE.; Ridgway, GR.; Tournier, JD.; Vaughan, DN.; Rose, S.; Henderson, R.; Connelly, A. Connectivity-based fixel enhancement: Whole-brain statistical analysis of diffusion MRI measures in the presence of crossing fibres.Neuroimage, 2015, 15(117):40-55

            * If not using the -cfe_legacy option:
    Smith, RE.; Dimond, D; Vaughan, D.; Parker, D.; Dhollander, T.; Jackson, G.; Connelly, A. Intrinsic non-stationarity correction for Fixel-Based Analysis. In Proc OHBM 2019 M789

            * If using the -nonstationary option:
    Salimi-Khorshidi, G. Smith, S.M. Nichols, T.E. Adjusting the effect of nonstationarity in cluster-based and TFCE inference. NeuroImage, 2011, 54(3), 2006-19

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: David Raffelt (david.raffelt@florey.edu.au) and Robert E. Smith (robert.smith@florey.edu.au)

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

    executable = "fixelcfestats"
    input_spec = fixelcfestats_input_spec
    output_spec = fixelcfestats_output_spec
