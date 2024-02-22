# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "fixel_directory",
        Directory,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the directory containing the fixels between which connectivity will be quantified""",
            "mandatory": True,
        },
    ),
    (
        "tracks",
        Tracks,
        {
            "argstr": "",
            "position": 1,
            "help_string": """the tracks used to determine fixel-fixel connectivity""",
            "mandatory": True,
        },
    ),
    (
        "matrix",
        Path,
        {
            "argstr": "",
            "position": 2,
            "output_file_template": "matrix",
            "help_string": """the output fixel-fixel connectivity matrix directory path""",
        },
    ),
    # Options that influence generation of the connectivity matrix / matrices Option Group
    (
        "threshold",
        float,
        {
            "argstr": "-threshold",
            "help_string": """a threshold to define the required fraction of shared connections to be included in the neighbourhood (default: 0.01)""",
        },
    ),
    (
        "angle",
        float,
        {
            "argstr": "-angle",
            "help_string": """the max angle threshold for assigning streamline tangents to fixels (Default: 45 degrees)""",
        },
    ),
    (
        "mask",
        ImageIn,
        {
            "argstr": "-mask",
            "help_string": """provide a fixel data file containing a mask of those fixels to be computed; fixels outside the mask will be empty in the output matrix""",
        },
    ),
    (
        "tck_weights_in",
        File,
        {
            "argstr": "-tck_weights_in",
            "help_string": """specify a text scalar file containing the streamline weights""",
        },
    ),
    # Options for additional outputs to be generated Option Group
    (
        "count",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-count",
            "output_file_template": "count.mif",
            "help_string": """export a fixel data file encoding the number of connections for each fixel""",
        },
    ),
    (
        "extent",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-extent",
            "output_file_template": "extent.mif",
            "help_string": """export a fixel data file encoding the extent of connectivity (sum of weights) for each fixel""",
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

fixelconnectivity_input_spec = specs.SpecInfo(
    name="fixelconnectivity_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "matrix",
        Directory,
        {
            "help_string": """the output fixel-fixel connectivity matrix directory path""",
        },
    ),
    (
        "count",
        ImageOut,
        {
            "help_string": """export a fixel data file encoding the number of connections for each fixel""",
        },
    ),
    (
        "extent",
        ImageOut,
        {
            "help_string": """export a fixel data file encoding the extent of connectivity (sum of weights) for each fixel""",
        },
    ),
]
fixelconnectivity_output_spec = specs.SpecInfo(
    name="fixelconnectivity_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class fixelconnectivity(ShellCommandTask):
    """This command will generate a directory containing three images, which encodes the fixel-fixel connectivity matrix. Documentation regarding this format and how to use it will come in the future.

        Fixel data are stored utilising the fixel directory format described in the main documentation, which can be found at the following link:
    https://mrtrix.readthedocs.io/en/3.0.4/fixel_based_analysis/fixel_directory_format.html


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

    executable = "fixelconnectivity"
    input_spec = fixelconnectivity_input_spec
    output_spec = fixelconnectivity_output_spec
