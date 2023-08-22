# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "tracks",
        File,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the input track file.""",
            "mandatory": True,
        },
    ),
    (
        "fmri",
        ImageIn,
        {
            "argstr": "",
            "position": 1,
            "help_string": """the pre-processed fMRI time series""",
            "mandatory": True,
        },
    ),
    (
        "output",
        Path,
        {
            "argstr": "",
            "position": 2,
            "output_file_template": "output.mif",
            "help_string": """the output TW-dFC image""",
        },
    ),
    # Options for toggling between static and dynamic TW-dFC methods; note that one of these options MUST be provided Option Group
    (
        "static",
        bool,
        {
            "argstr": "-static",
            "help_string": """generate a "static" (3D) output image.""",
        },
    ),
    (
        "dynamic",
        ty.Tuple[str, str],
        {
            "argstr": "-dynamic",
            "help_string": """generate a "dynamic" (4D) output image; must additionally provide the shape and width (in volumes) of the sliding window.""",
        },
    ),
    # Options for setting the properties of the output image Option Group
    (
        "template",
        ImageIn,
        {
            "argstr": "-template",
            "help_string": """an image file to be used as a template for the output (the output image will have the same transform and field of view).""",
        },
    ),
    (
        "vox",
        ty.List[float],
        {
            "argstr": "-vox",
            "help_string": """provide either an isotropic voxel size (in mm), or comma-separated list of 3 voxel dimensions.""",
            "sep": ",",
        },
    ),
    (
        "stat_vox",
        str,
        {
            "argstr": "-stat_vox",
            "help_string": """define the statistic for choosing the final voxel intensities for a given contrast type given the individual values from the tracks passing through each voxel
Options are: sum, min, mean, max (default: mean)""",
            "allowed_values": ["sum", "sum", "min", "mean", "max"],
        },
    ),
    # Other options for affecting the streamline sampling & mapping behaviour Option Group
    (
        "backtrack",
        bool,
        {
            "argstr": "-backtrack",
            "help_string": """if no valid timeseries is found at the streamline endpoint, back-track along the streamline trajectory until a valid timeseries is found""",
        },
    ),
    (
        "upsample",
        int,
        {
            "argstr": "-upsample",
            "help_string": """upsample the tracks by some ratio using Hermite interpolation before mapping (if omitted, an appropriate ratio will be determined automatically)""",
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

tckdfc_input_spec = specs.SpecInfo(
    name="tckdfc_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "output",
        ImageOut,
        {
            "help_string": """the output TW-dFC image""",
        },
    ),
]
tckdfc_output_spec = specs.SpecInfo(
    name="tckdfc_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class tckdfc(ShellCommandTask):
    """This command generates a Track-Weighted Image (TWI), where the contribution from each streamline to the image is the Pearson correlation between the fMRI time series at the streamline endpoints.

        The output image can be generated in one of two ways (note that one of these two command-line options MUST be provided):

        - "Static" functional connectivity (-static option): Each streamline contributes to a static 3D output image based on the correlation between the signals at the streamline endpoints using the entirety of the input time series.

        - "Dynamic" functional connectivity (-dynamic option): The output image is a 4D image, with the same number of volumes as the input fMRI time series. For each volume, the contribution from each streamline is calculated based on a finite-width sliding time window, centred at the timepoint corresponding to that volume.

        Note that the -backtrack option in this command is similar, but not precisely equivalent, to back-tracking as can be used with Anatomically-Constrained Tractography (ACT) in the tckgen command. However, here the feature does not change the streamlines trajectories in any way; it simply enables detection of the fact that the input fMRI image may not contain a valid timeseries underneath the streamline endpoint, and where this occurs, searches from the streamline endpoint inwards along the streamline trajectory in search of a valid timeseries to sample from the input image.


        References
        ----------

            Calamante, F.; Smith, R.E.; Liang, X.; Zalesky, A.; Connelly, A Track-weighted dynamic functional connectivity (TW-dFC): a new method to study time-resolved functional connectivity. Brain Struct Funct, 2017, doi: 10.1007/s00429-017-1431-1

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

    executable = "tckdfc"
    input_spec = tckdfc_input_spec
    output_spec = tckdfc_output_spec
