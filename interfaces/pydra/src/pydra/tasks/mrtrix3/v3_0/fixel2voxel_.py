# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "fixel_in",
        ImageIn,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the input fixel data file""",
            "mandatory": True,
        },
    ),
    (
        "operation",
        str,
        {
            "argstr": "",
            "position": 1,
            "help_string": """the operation to apply, one of: mean, sum, product, min, max, absmax, magmax, count, complexity, sf, dec_unit, dec_scaled, none.""",
            "mandatory": True,
            "allowed_values": [
                "mean",
                "mean",
                "sum",
                "product",
                "min",
                "max",
                "absmax",
                "magmax",
                "count",
                "complexity",
                "sf",
                "dec_unit",
                "dec_scaled",
                "none",
            ],
        },
    ),
    (
        "image_out",
        Path,
        {
            "argstr": "",
            "position": 2,
            "output_file_template": "image_out.mif",
            "help_string": """the output scalar image.""",
        },
    ),
    (
        "number",
        int,
        {
            "argstr": "-number",
            "help_string": """use only the largest N fixels in calculation of the voxel-wise statistic; in the case of operation "none", output only the largest N fixels in each voxel.""",
        },
    ),
    (
        "fill",
        float,
        {
            "argstr": "-fill",
            "help_string": """for "none" operation, specify the value to fill when number of fixels is fewer than the maximum (default: 0.0)""",
        },
    ),
    (
        "weighted",
        ImageIn,
        {
            "argstr": "-weighted",
            "help_string": """weight the contribution of each fixel to the per-voxel result according to its volume.""",
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

fixel2voxel_input_spec = specs.SpecInfo(
    name="fixel2voxel_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "image_out",
        ImageOut,
        {
            "help_string": """the output scalar image.""",
        },
    ),
]
fixel2voxel_output_spec = specs.SpecInfo(
    name="fixel2voxel_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class fixel2voxel(ShellCommandTask):
    """Fixel data can be reduced to voxel data in a number of ways:

        - Some statistic computed across all fixel values within a voxel: mean, sum, product, min, max, absmax, magmax

        - The number of fixels in each voxel: count

        - Some measure of crossing-fibre organisation: complexity, sf ('single-fibre')

        - A 4D directionally-encoded colour image: dec_unit, dec_scaled

        - A 4D image containing all fixel data values in each voxel unmodified: none

        The -weighted option deals with the case where there is some per-fixel metric of interest that you wish to collapse into a single scalar measure per voxel, but each fixel possesses a different volume, and you wish for those fixels with greater volume to have a greater influence on the calculation than fixels with lesser volume. For instance, when estimating a voxel-based measure of mean axon diameter from per-fixel mean axon diameters, a fixel's mean axon diameter should be weigthed by its relative volume within the voxel in the calculation of that voxel mean.

        Fixel data are stored utilising the fixel directory format described in the main documentation, which can be found at the following link:
    https://mrtrix.readthedocs.io/en/3.0.4/fixel_based_analysis/fixel_directory_format.html


        References
        ----------

            * Reference for 'complexity' operation:
    Riffert, T. W.; Schreiber, J.; Anwander, A. & Knosche, T. R. Beyond Fractional Anisotropy: Extraction of bundle-specific structural metrics from crossing fibre models. NeuroImage, 2014, 100, 176-191

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: Robert E. Smith (robert.smith@florey.edu.au) & David Raffelt (david.raffelt@florey.edu.au)

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

    executable = "fixel2voxel"
    input_spec = fixel2voxel_input_spec
    output_spec = fixel2voxel_output_spec
