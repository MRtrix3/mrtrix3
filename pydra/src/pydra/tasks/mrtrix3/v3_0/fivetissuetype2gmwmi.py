import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.mrtrix3 import TrackFile  # noqa: F401
from pydra import ShellCommandTask
from pydra.engine import specs
from pydra.tasks.mrtrix3.fileformats import ImageIn, ImageOut  # noqa: F401


input_fields = [
    # Arguments
    (
        "in_5tt",
        ImageIn,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the input 5TT segmented anatomical image""",
            "mandatory": True,
        },
    ),
    (
        "mask_out",
        Path,
        {
            "argstr": "",
            "position": 1,
            "output_file_template": "mask_out.mif",
            "help_string": """the output mask image""",
        },
    ),
    (
        "mask_in",
        ImageIn,
        {
            "argstr": "-mask_in",
            "help_string": """Filter an input mask image according to those voxels that lie upon the grey matter - white matter boundary. If no input mask is provided, the output will be a whole-brain mask image calculated using the anatomical image only.""",
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

fivetissuetype2gmwmi_input_spec = specs.SpecInfo(
    name="Input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "mask_out",
        ImageOut,
        {
            "help_string": """the output mask image""",
        },
    ),
]
fivetissuetype2gmwmi_output_spec = specs.SpecInfo(
    name="Output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class fivetissuetype2gmwmi(ShellCommandTask):
    """
        References
        ----------

            Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. Anatomically-constrained tractography:Improved diffusion MRI streamlines tractography through effective use of anatomical information. NeuroImage, 2012, 62, 1924-1938

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-624-g4fb87551-dirty, built Aug 21 2023

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

    executable = "fivetissuetype2gmwmi"
    input_spec = fivetissuetype2gmwmi_input_spec
    output_spec = fivetissuetype2gmwmi_output_spec
