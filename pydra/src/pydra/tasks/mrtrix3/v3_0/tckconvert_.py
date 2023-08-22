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
        ty.Any,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the input track file.""",
            "mandatory": True,
        },
    ),
    (
        "output",
        Path,
        {
            "argstr": "",
            "position": 1,
            "output_file_template": "output.txt",
            "help_string": """the output track file.""",
        },
    ),
    (
        "scanner2voxel",
        ImageIn,
        {
            "argstr": "-scanner2voxel",
            "help_string": """if specified, the properties of this image will be used to convert track point positions from real (scanner) coordinates into voxel coordinates.""",
        },
    ),
    (
        "scanner2image",
        ImageIn,
        {
            "argstr": "-scanner2image",
            "help_string": """if specified, the properties of this image will be used to convert track point positions from real (scanner) coordinates into image coordinates (in mm).""",
        },
    ),
    (
        "voxel2scanner",
        ImageIn,
        {
            "argstr": "-voxel2scanner",
            "help_string": """if specified, the properties of this image will be used to convert track point positions from voxel coordinates into real (scanner) coordinates.""",
        },
    ),
    (
        "image2scanner",
        ImageIn,
        {
            "argstr": "-image2scanner",
            "help_string": """if specified, the properties of this image will be used to convert track point positions from image coordinates (in mm) into real (scanner) coordinates.""",
        },
    ),
    # Options specific to PLY writer Option Group
    (
        "sides",
        int,
        {
            "argstr": "-sides",
            "help_string": """number of sides for streamlines""",
        },
    ),
    (
        "increment",
        int,
        {
            "argstr": "-increment",
            "help_string": """generate streamline points at every (increment) points""",
        },
    ),
    # Options specific to RIB writer Option Group
    (
        "dec",
        bool,
        {
            "argstr": "-dec",
            "help_string": """add DEC as a primvar""",
        },
    ),
    # Options for both PLY and RIB writer Option Group
    (
        "radius",
        float,
        {
            "argstr": "-radius",
            "help_string": """radius of the streamlines""",
        },
    ),
    # Options specific to VTK writer Option Group
    (
        "ascii",
        bool,
        {
            "argstr": "-ascii",
            "help_string": """write an ASCII VTK file (binary by default)""",
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

tckconvert_input_spec = specs.SpecInfo(
    name="tckconvert_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "output",
        File,
        {
            "help_string": """the output track file.""",
        },
    ),
]
tckconvert_output_spec = specs.SpecInfo(
    name="tckconvert_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class tckconvert(ShellCommandTask):
    """The program currently supports MRtrix .tck files (input/output), ascii text files (input/output), VTK polydata files (input/output), and RenderMan RIB (export only).

        Note that ascii files will be stored with one streamline per numbered file. To support this, the command will use the multi-file numbering syntax, where square brackets denote the position of the numbering for the files, for example:

        $ tckconvert input.tck output-'[]'.txt

        will produce files named output-0000.txt, output-0001.txt, output-0002.txt, ...


        References
        ----------

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: Daan Christiaens (daan.christiaens@kcl.ac.uk), J-Donald Tournier (jdtournier@gmail.com), Philip Broser (philip.broser@me.com), Daniel Blezek (daniel.blezek@gmail.com).

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

    executable = "tckconvert"
    input_spec = tckconvert_input_spec
    output_spec = tckconvert_output_spec
