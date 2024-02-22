# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "in_",
        ImageIn,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the input warp image.""",
            "mandatory": True,
        },
    ),
    (
        "type",
        str,
        {
            "argstr": "",
            "position": 1,
            "help_string": """the conversion type required. Valid choices are: deformation2displacement, displacement2deformation, warpfull2deformation, warpfull2displacement""",
            "mandatory": True,
            "allowed_values": [
                "deformation2displacement",
                "deformation2displacement",
                "displacement2deformation",
                "warpfull2deformation",
                "warpfull2displacement",
            ],
        },
    ),
    (
        "out",
        Path,
        {
            "argstr": "",
            "position": 2,
            "output_file_template": "out.mif",
            "help_string": """the output warp image.""",
        },
    ),
    (
        "template",
        ImageIn,
        {
            "argstr": "-template",
            "help_string": """define a template image when converting a warpfull file (which is defined on a grid in the midway space between image 1 & 2). For example to generate the deformation field that maps image1 to image2, then supply image2 as the template image""",
        },
    ),
    (
        "midway_space",
        bool,
        {
            "argstr": "-midway_space",
            "help_string": """to be used only with warpfull2deformation and warpfull2displacement conversion types. The output will only contain the non-linear warp to map an input image to the midway space (defined by the warpfull grid). If a linear transform exists in the warpfull file header then it will be composed and included in the output.""",
        },
    ),
    (
        "from_",
        int,
        {
            "argstr": "-from",
            "help_string": """to be used only with warpfull2deformation and warpfull2displacement conversion types. Used to define the direction of the desired output field.Use -from 1 to obtain the image1->image2 field and from 2 for image2->image1. Can be used in combination with the -midway_space option to produce a field that only maps to midway space.""",
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

warpconvert_input_spec = specs.SpecInfo(
    name="warpconvert_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "out",
        ImageOut,
        {
            "help_string": """the output warp image.""",
        },
    ),
]
warpconvert_output_spec = specs.SpecInfo(
    name="warpconvert_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class warpconvert(ShellCommandTask):
    """A deformation field is defined as an image where each voxel defines the corresponding position in the other image (in scanner space coordinates). A displacement field stores the displacements (in mm) to the other image from the each voxel's position (in scanner space). The warpfull file is the 5D format output from mrregister -nl_warp_full, which contains linear transforms, warps and their inverses that map each image to a midway space.


        References
        ----------

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: David Raffelt (david.raffelt@florey.edu.au)

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

    executable = "warpconvert"
    input_spec = warpconvert_input_spec
    output_spec = warpconvert_output_spec
