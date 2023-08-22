# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "image_",
        specs.MultiInputObj[ImageIn],
        {
            "argstr": "",
            "position": 0,
            "help_string": """the input image(s).""",
            "mandatory": True,
        },
    ),
    (
        "all",
        bool,
        {
            "argstr": "-all",
            "help_string": """print all properties, rather than the first and last 2 of each.""",
        },
    ),
    (
        "name",
        bool,
        {
            "argstr": "-name",
            "help_string": """print the file system path of the image""",
        },
    ),
    (
        "format",
        bool,
        {
            "argstr": "-format",
            "help_string": """image file format""",
        },
    ),
    (
        "ndim",
        bool,
        {
            "argstr": "-ndim",
            "help_string": """number of image dimensions""",
        },
    ),
    (
        "size",
        bool,
        {
            "argstr": "-size",
            "help_string": """image size along each axis""",
        },
    ),
    (
        "spacing",
        bool,
        {
            "argstr": "-spacing",
            "help_string": """voxel spacing along each image dimension""",
        },
    ),
    (
        "datatype",
        bool,
        {
            "argstr": "-datatype",
            "help_string": """data type used for image data storage""",
        },
    ),
    (
        "strides",
        bool,
        {
            "argstr": "-strides",
            "help_string": """data strides i.e. order and direction of axes data layout""",
        },
    ),
    (
        "offset",
        bool,
        {
            "argstr": "-offset",
            "help_string": """image intensity offset""",
        },
    ),
    (
        "multiplier",
        bool,
        {
            "argstr": "-multiplier",
            "help_string": """image intensity multiplier""",
        },
    ),
    (
        "transform",
        bool,
        {
            "argstr": "-transform",
            "help_string": """the transformation from image coordinates [mm] to scanner / real world coordinates [mm]""",
        },
    ),
    # Options for exporting image header fields Option Group
    (
        "property",
        specs.MultiInputObj[str],
        {
            "argstr": "-property",
            "help_string": """any text properties embedded in the image header under the specified key (use 'all' to list all keys found)""",
        },
    ),
    (
        "json_keyval",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-json_keyval",
            "output_file_template": "json_keyval.txt",
            "help_string": """export header key/value entries to a JSON file""",
        },
    ),
    (
        "json_all",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-json_all",
            "output_file_template": "json_all.txt",
            "help_string": """export all header contents to a JSON file""",
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
    (
        "bvalue_scaling",
        bool,
        {
            "argstr": "-bvalue_scaling",
            "help_string": """enable or disable scaling of diffusion b-values by the square of the corresponding DW gradient norm (see Desciption). Valid choices are yes/no, true/false, 0/1 (default: automatic).""",
        },
    ),
    # DW gradient table export options Option Group
    (
        "export_grad_mrtrix",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-export_grad_mrtrix",
            "output_file_template": "export_grad_mrtrix.txt",
            "help_string": """export the diffusion-weighted gradient table to file in MRtrix format""",
        },
    ),
    (
        "export_grad_fsl",
        ty.Union[ty.Tuple[Path, Path], bool],
        False,
        {
            "argstr": "-export_grad_fsl",
            "output_file_template": (
                "export_grad_fsl0.txt",
                "export_grad_fsl1.txt",
            ),
            "help_string": """export the diffusion-weighted gradient table to files in FSL (bvecs / bvals) format""",
        },
    ),
    (
        "dwgrad",
        bool,
        {
            "argstr": "-dwgrad",
            "help_string": """the diffusion-weighting gradient table, as interpreted by MRtrix3""",
        },
    ),
    (
        "shell_bvalues",
        bool,
        {
            "argstr": "-shell_bvalues",
            "help_string": """list the average b-value of each shell""",
        },
    ),
    (
        "shell_sizes",
        bool,
        {
            "argstr": "-shell_sizes",
            "help_string": """list the number of volumes in each shell""",
        },
    ),
    (
        "shell_indices",
        bool,
        {
            "argstr": "-shell_indices",
            "help_string": """list the image volumes attributed to each b-value shell""",
        },
    ),
    # Options for exporting phase-encode tables Option Group
    (
        "export_pe_table",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-export_pe_table",
            "output_file_template": "export_pe_table.txt",
            "help_string": """export phase-encoding table to file""",
        },
    ),
    (
        "export_pe_eddy",
        ty.Union[ty.Tuple[Path, Path], bool],
        False,
        {
            "argstr": "-export_pe_eddy",
            "output_file_template": (
                "export_pe_eddy0.txt",
                "export_pe_eddy1.txt",
            ),
            "help_string": """export phase-encoding information to an EDDY-style config / index file pair""",
        },
    ),
    (
        "petable",
        bool,
        {
            "argstr": "-petable",
            "help_string": """print the phase encoding table""",
        },
    ),
    # Handling of piped images Option Group
    (
        "nodelete",
        bool,
        {
            "argstr": "-nodelete",
            "help_string": """don't delete temporary images or images passed to mrinfo via Unix pipes""",
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

mrinfo_input_spec = specs.SpecInfo(
    name="mrinfo_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "json_keyval",
        File,
        {
            "help_string": """export header key/value entries to a JSON file""",
        },
    ),
    (
        "json_all",
        File,
        {
            "help_string": """export all header contents to a JSON file""",
        },
    ),
    (
        "export_grad_mrtrix",
        File,
        {
            "help_string": """export the diffusion-weighted gradient table to file in MRtrix format""",
        },
    ),
    (
        "export_grad_fsl",
        ty.Tuple[File, File],
        {
            "help_string": """export the diffusion-weighted gradient table to files in FSL (bvecs / bvals) format""",
        },
    ),
    (
        "export_pe_table",
        File,
        {
            "help_string": """export phase-encoding table to file""",
        },
    ),
    (
        "export_pe_eddy",
        ty.Tuple[File, File],
        {
            "help_string": """export phase-encoding information to an EDDY-style config / index file pair""",
        },
    ),
]
mrinfo_output_spec = specs.SpecInfo(
    name="mrinfo_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class mrinfo(ShellCommandTask):
    """By default, all information contained in each image header will be printed to the console in a reader-friendly format.

        Alternatively, command-line options may be used to extract specific details from the header(s); these are printed to the console in a format more appropriate for scripting purposes or piping to file. If multiple options and/or images are provided, the requested header fields will be printed in the order in which they appear in the help page, with all requested details from each input image in sequence printed before the next image is processed.

        The command can also write the diffusion gradient table from a single input image to file; either in the MRtrix or FSL format (bvecs/bvals file pair; includes appropriate diffusion gradient vector reorientation)

        The -dwgrad, -export_* and -shell_* options provide (information about) the diffusion weighting gradient table after it has been processed by the MRtrix3 back-end (vectors normalised, b-values scaled by the square of the vector norm, depending on the -bvalue_scaling option). To see the raw gradient table information as stored in the image header, i.e. without MRtrix3 back-end processing, use "-property dw_scheme".

        The -bvalue_scaling option controls an aspect of the import of diffusion gradient tables. When the input diffusion-weighting direction vectors have norms that differ substantially from unity, the b-values will be scaled by the square of their corresponding vector norm (this is how multi-shell acquisitions are frequently achieved on scanner platforms). However in some rare instances, the b-values may be correct, despite the vectors not being of unit norm (or conversely, the b-values may need to be rescaled even though the vectors are close to unit norm). This option allows the user to control this operation and override MRrtix3's automatic detection.


        References
        ----------

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: J-Donald Tournier (d.tournier@brain.org.au) and Robert E. Smith (robert.smith@florey.edu.au)

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

    executable = "mrinfo"
    input_spec = mrinfo_input_spec
    output_spec = mrinfo_output_spec
