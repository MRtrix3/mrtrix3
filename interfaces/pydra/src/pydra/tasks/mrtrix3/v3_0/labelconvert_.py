# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "path_in",
        ImageIn,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the input image""",
            "mandatory": True,
        },
    ),
    (
        "lut_in",
        File,
        {
            "argstr": "",
            "position": 1,
            "help_string": """the connectome lookup table corresponding to the input image""",
            "mandatory": True,
        },
    ),
    (
        "lut_out",
        File,
        {
            "argstr": "",
            "position": 2,
            "help_string": """the target connectome lookup table for the output image""",
            "mandatory": True,
        },
    ),
    (
        "image_out",
        Path,
        {
            "argstr": "",
            "position": 3,
            "output_file_template": "image_out.mif",
            "help_string": """the output image""",
        },
    ),
    (
        "spine",
        ImageIn,
        {
            "argstr": "-spine",
            "help_string": """provide a manually-defined segmentation of the base of the spine where the streamlines terminate, so that this can become a node in the connection matrix.""",
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

labelconvert_input_spec = specs.SpecInfo(
    name="labelconvert_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "image_out",
        ImageOut,
        {
            "help_string": """the output image""",
        },
    ),
]
labelconvert_output_spec = specs.SpecInfo(
    name="labelconvert_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class labelconvert(ShellCommandTask):
    """Typical usage is to convert a parcellation image provided by some other software, based on the lookup table provided by that software, to conform to a new lookup table, particularly one where the node indices increment from 1, in preparation for connectome construction; examples of such target lookup table files are provided in share//mrtrix3//labelconvert//, but can be created by the user to provide the desired node set // ordering // colours.

        A more thorough description of the operation and purpose of the labelconvert command can be found in the online documentation:
    https://mrtrix.readthedocs.io/en/3.0.4/quantitative_structural_connectivity/labelconvert_tutorial.html


        Example usages
        --------------


        Convert a Desikan-Killiany parcellation image as provided by FreeSurfer to have nodes incrementing from 1:

            `$ labelconvert aparc+aseg.mgz FreeSurferColorLUT.txt mrtrix3//share//mrtrix3//labelconvert//fs_default.txt nodes.mif`

            Paths to the files in the example above would need to be revised according to their locations on the user's system.


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

    executable = "labelconvert"
    input_spec = labelconvert_input_spec
    output_spec = labelconvert_output_spec
