# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "input_output",
        specs.MultiInputObj[ty.Any],
        {
            "argstr": "",
            "position": 0,
            "help_string": """list of all input and output tissue compartment files (see example usage).""",
            "mandatory": True,
        },
    ),
    (
        "mask",
        ImageIn,
        {
            "argstr": "-mask",
            "help_string": """the mask defines the data used to compute the intensity normalisation. This option is mandatory.""",
            "mandatory": True,
        },
    ),
    (
        "order",
        str,
        {
            "argstr": "-order",
            "help_string": """the maximum order of the polynomial basis used to fit the normalisation field in the log-domain. An order of 0 is equivalent to not allowing spatial variance of the intensity normalisation factor. (default: 3)""",
            "allowed_values": ["0", "0", "1", "2", "3"],
        },
    ),
    (
        "niter",
        ty.List[int],
        {
            "argstr": "-niter",
            "help_string": """set the number of iterations. The first (and potentially only) entry applies to the main loop. If supplied as a comma-separated list of integers, the second entry applies to the inner loop to update the balance factors (default: 15,7).""",
            "sep": ",",
        },
    ),
    (
        "reference",
        float,
        {
            "argstr": "-reference",
            "help_string": """specify the (positive) reference value to which the summed tissue compartments will be normalised. (default: 0.282095, SH DC term for unit angular integral)""",
        },
    ),
    (
        "balanced",
        bool,
        {
            "argstr": "-balanced",
            "help_string": """incorporate the per-tissue balancing factors into scaling of the output images (NOTE: use of this option has critical consequences for AFD intensity normalisation; should not be used unless these consequences are fully understood)""",
        },
    ),
    # Debugging options Option Group
    (
        "check_norm",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-check_norm",
            "output_file_template": "check_norm.mif",
            "help_string": """output the final estimated spatially varying intensity level that is used for normalisation.""",
        },
    ),
    (
        "check_mask",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-check_mask",
            "output_file_template": "check_mask.mif",
            "help_string": """output the final mask used to compute the normalisation. This mask excludes regions identified as outliers by the optimisation process.""",
        },
    ),
    (
        "check_factors",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-check_factors",
            "output_file_template": "check_factors.txt",
            "help_string": """output the tissue balance factors computed during normalisation.""",
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

mtnormalise_input_spec = specs.SpecInfo(
    name="mtnormalise_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "check_norm",
        ImageOut,
        {
            "help_string": """output the final estimated spatially varying intensity level that is used for normalisation.""",
        },
    ),
    (
        "check_mask",
        ImageOut,
        {
            "help_string": """output the final mask used to compute the normalisation. This mask excludes regions identified as outliers by the optimisation process.""",
        },
    ),
    (
        "check_factors",
        File,
        {
            "help_string": """output the tissue balance factors computed during normalisation.""",
        },
    ),
]
mtnormalise_output_spec = specs.SpecInfo(
    name="mtnormalise_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class mtnormalise(ShellCommandTask):
    """This command takes as input any number of tissue components (e.g. from multi-tissue CSD) and outputs corresponding normalised tissue components corrected for the effects of (residual) intensity inhomogeneities. Intensity normalisation is performed by optimising the voxel-wise sum of all tissue compartments towards a constant value, under constraints of spatial smoothness (polynomial basis of a given order). Different to the Raffelt et al. 2017 abstract, this algorithm performs this task in the log-domain instead, with added gradual outlier rejection, different handling of the balancing factors between tissue compartments and a different iteration structure.

        The -mask option is mandatory and is optimally provided with a brain mask (such as the one obtained from dwi2mask earlier in the processing pipeline). Outlier areas with exceptionally low or high combined tissue contributions are accounted for and reoptimised as the intensity inhomogeneity estimation becomes more accurate.


        Example usages
        --------------


        Default usage (for 3-tissue CSD compartments):

            `$ mtnormalise wmfod.mif wmfod_norm.mif gm.mif gm_norm.mif csf.mif csf_norm.mif -mask mask.mif`

            Note how for each tissue compartment, the input and output images are provided as a consecutive pair.


        References
        ----------

            Raffelt, D.; Dhollander, T.; Tournier, J.-D.; Tabbara, R.; Smith, R. E.; Pierre, E. & Connelly, A. Bias Field Correction and Intensity Normalisation for Quantitative Analysis of Apparent Fibre Density. In Proc. ISMRM, 2017, 26, 3541

            Dhollander, T.; Tabbara, R.; Rosnarho-Tornstrand, J.; Tournier, J.-D.; Raffelt, D. & Connelly, A. Multi-tissue log-domain intensity and inhomogeneity normalisation for quantitative apparent fibre density. In Proc. ISMRM, 2021, 29, 2472

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: Thijs Dhollander (thijs.dhollander@gmail.com), Rami Tabbara (rami.tabbara@florey.edu.au), David Raffelt (david.raffelt@florey.edu.au), Jonas Rosnarho-Tornstrand (jonas.rosnarho-tornstrand@kcl.ac.uk) and J-Donald Tournier (jdtournier@gmail.com)

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

    executable = "mtnormalise"
    input_spec = mtnormalise_input_spec
    output_spec = mtnormalise_output_spec
