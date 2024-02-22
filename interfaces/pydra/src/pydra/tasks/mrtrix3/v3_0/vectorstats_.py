# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "input",
        File,
        {
            "argstr": "",
            "position": 0,
            "help_string": """a text file listing the file names of the input subject data""",
            "mandatory": True,
        },
    ),
    (
        "design",
        File,
        {
            "argstr": "",
            "position": 1,
            "help_string": """the design matrix""",
            "mandatory": True,
        },
    ),
    (
        "contrast",
        File,
        {
            "argstr": "",
            "position": 2,
            "help_string": """the contrast matrix""",
            "mandatory": True,
        },
    ),
    (
        "output",
        str,
        {
            "argstr": "",
            "position": 3,
            "help_string": """the filename prefix for all output""",
            "mandatory": True,
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
            "help_string": """add a column to the design matrix corresponding to subject element-wise values (note that the contrast matrix must include an additional column for each use of this option); the text file provided via this option should contain a file name for each subject""",
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

vectorstats_input_spec = specs.SpecInfo(
    name="vectorstats_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = []
vectorstats_output_spec = specs.SpecInfo(
    name="vectorstats_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class vectorstats(ShellCommandTask):
    """This command can be used to perform permutation testing of any form of data. The data for each input subject must be stored in a text file, with one value per row. The data for each row across subjects will be tested independently, i.e. there is no statistical enhancement that occurs between the data; however family-wise error control will be used.

        In some software packages, a column of ones is automatically added to the GLM design matrix; the purpose of this column is to estimate the "global intercept", which is the predicted value of the observed variable if all explanatory variables were to be zero. However there are rare situations where including such a column would not be appropriate for a particular experimental design. Hence, in MRtrix3 statistical inference commands, it is up to the user to determine whether or not this column of ones should be included in their design matrix, and add it explicitly if necessary. The contrast matrix must also reflect the presence of this additional column.


        References
        ----------

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

    executable = "vectorstats"
    input_spec = vectorstats_input_spec
    output_spec = vectorstats_output_spec
