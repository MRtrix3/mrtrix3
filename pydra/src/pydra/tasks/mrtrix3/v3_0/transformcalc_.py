# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "inputs",
        specs.MultiInputObj[ty.Any],
        {
            "argstr": "",
            "position": 0,
            "help_string": """the input(s) for the specified operation""",
            "mandatory": True,
        },
    ),
    (
        "operation",
        str,
        {
            "argstr": "",
            "position": 1,
            "help_string": """the operation to perform, one of: invert, half, rigid, header, average, interpolate, decompose, align_vertices_rigid, align_vertices_rigid_scale (see description section for details).""",
            "mandatory": True,
            "allowed_values": [
                "invert",
                "invert",
                "half",
                "rigid",
                "header",
                "average",
                "interpolate",
                "decompose",
                "align_vertices_rigid",
                "align_vertices_rigid_scale",
            ],
        },
    ),
    (
        "output",
        Path,
        {
            "argstr": "",
            "position": 2,
            "output_file_template": "output.txt",
            "help_string": """the output transformation matrix.""",
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

transformcalc_input_spec = specs.SpecInfo(
    name="transformcalc_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "output",
        File,
        {
            "help_string": """the output transformation matrix.""",
        },
    ),
]
transformcalc_output_spec = specs.SpecInfo(
    name="transformcalc_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class transformcalc(ShellCommandTask):
    """
        Example usages
        --------------


        Invert a transformation:

            `$ transformcalc matrix_in.txt invert matrix_out.txt`



        Calculate the matrix square root of the input transformation (halfway transformation):

            `$ transformcalc matrix_in.txt half matrix_out.txt`



        Calculate the rigid component of an affine input transformation:

            `$ transformcalc affine_in.txt rigid rigid_out.txt`



        Calculate the transformation matrix from an original image and an image with modified header:

            `$ transformcalc mov mapmovhdr header output`



        Calculate the average affine matrix of a set of input matrices:

            `$ transformcalc input1.txt ... inputN.txt average matrix_out.txt`



        Create interpolated transformation matrix between two inputs:

            `$ transformcalc input1.txt input2.txt interpolate matrix_out.txt`

            Based on matrix decomposition with linear interpolation of translation, rotation and stretch described in: Shoemake, K., Hill, M., & Duff, T. (1992). Matrix Animation and Polar Decomposition. Matrix, 92, 258-264. doi:10.1.1.56.1336


        Decompose transformation matrix M into translation, rotation and stretch and shear (M = T * R * S):

            `$ transformcalc matrix_in.txt decompose matrixes_out.txt`

            The output is a key-value text file containing: scaling: vector of 3 scaling factors in x, y, z direction; shear: list of shear factors for xy, xz, yz axes; angles: list of Euler angles about static x, y, z axes in radians in the range [0:pi]x[-pi:pi]x[-pi:pi]; angle_axis: angle in radians and rotation axis; translation : translation vector along x, y, z axes in mm; R: composed roation matrix (R = rot_x * rot_y * rot_z); S: composed scaling and shear matrix


        Calculate transformation that aligns two images based on sets of corresponding landmarks:

            `$ transformcalc input moving.txt fixed.txt align_vertices_rigid rigid.txt`

            Similary, 'align_vertices_rigid_scale' produces an affine matrix (rigid and global scale). Vertex coordinates are in scanner space, corresponding vertices must be stored in the same row of moving.txt and fixed.txt. Requires 3 or more vertices in each file. Algorithm: Kabsch 'A solution for the best rotation to relate two sets of vectors' DOI:10.1107/S0567739476001873


        References
        ----------

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: Max Pietsch (maximilian.pietsch@kcl.ac.uk)

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

    executable = "transformcalc"
    input_spec = transformcalc_input_spec
    output_spec = transformcalc_output_spec
