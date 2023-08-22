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
        ImageIn,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the input DW image.""",
            "mandatory": True,
        },
    ),
    (
        "output",
        Path,
        {
            "argstr": "",
            "position": 1,
            "output_file_template": "output.mif",
            "help_string": """the output image (diffusion-weighted volumes by default).""",
        },
    ),
    (
        "bzero",
        bool,
        {
            "argstr": "-bzero",
            "help_string": """Output b=0 volumes (instead of the diffusion weighted volumes, if -singleshell is not specified).""",
        },
    ),
    (
        "no_bzero",
        bool,
        {
            "argstr": "-no_bzero",
            "help_string": """Output only non b=0 volumes (default, if -singleshell is not specified).""",
        },
    ),
    (
        "singleshell",
        bool,
        {
            "argstr": "-singleshell",
            "help_string": """Force a single-shell (single non b=0 shell) output. This will include b=0 volumes, if present. Use with -bzero to enforce presence of b=0 volumes (error if not present) or with -no_bzero to exclude them.""",
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
    # DW shell selection options Option Group
    (
        "shells",
        ty.List[float],
        {
            "argstr": "-shells",
            "help_string": """specify one or more b-values to use during processing, as a comma-separated list of the desired approximate b-values (b-values are clustered to allow for small deviations). Note that some commands are incompatible with multiple b-values, and will report an error if more than one b-value is provided. 
WARNING: note that, even though the b=0 volumes are never referred to as shells in the literature, they still have to be explicitly included in the list of b-values as provided to the -shell option! Several algorithms which include the b=0 volumes in their computations may otherwise return an undesired result.""",
            "sep": ",",
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
    # Options for importing phase-encode tables Option Group
    (
        "import_pe_table",
        File,
        {
            "argstr": "-import_pe_table",
            "help_string": """import a phase-encoding table from file""",
        },
    ),
    (
        "import_pe_eddy",
        ty.Tuple[File, File],
        {
            "argstr": "-import_pe_eddy",
            "help_string": """import phase-encoding information from an EDDY-style config / index file pair""",
        },
    ),
    # Options for selecting volumes based on phase-encoding Option Group
    (
        "pe",
        ty.List[float],
        {
            "argstr": "-pe",
            "help_string": """select volumes with a particular phase encoding; this can be three comma-separated values (for i,j,k components of vector direction) or four (direction & total readout time)""",
            "sep": ",",
        },
    ),
    # Stride options Option Group
    (
        "strides",
        ty.Any,
        {
            "argstr": "-strides",
            "help_string": """specify the strides of the output data in memory; either as a comma-separated list of (signed) integers, or as a template image from which the strides shall be extracted and used. The actual strides produced will depend on whether the output image format can support it.""",
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

dwiextract_input_spec = specs.SpecInfo(
    name="dwiextract_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "output",
        ImageOut,
        {
            "help_string": """the output image (diffusion-weighted volumes by default).""",
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
]
dwiextract_output_spec = specs.SpecInfo(
    name="dwiextract_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class dwiextract(ShellCommandTask):
    """
        Example usages
        --------------


        Calculate the mean b=0 image from a 4D DWI series:

            `$ dwiextract dwi.mif - -bzero | mrmath - mean mean_bzero.mif -axis 3`

            The dwiextract command extracts all volumes for which the b-value is (approximately) zero; the resulting 4D image can then be provided to the mrmath command to calculate the mean intensity across volumes for each voxel.


        References
        ----------

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: David Raffelt (david.raffelt@florey.edu.au) and Thijs Dhollander (thijs.dhollander@gmail.com) and Robert E. Smith (robert.smith@florey.edu.au)

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

    executable = "dwiextract"
    input_spec = dwiextract_input_spec
    output_spec = dwiextract_output_spec
