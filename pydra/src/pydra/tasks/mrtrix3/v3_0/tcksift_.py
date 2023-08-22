# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "in_tracks",
        Tracks,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the input track file""",
            "mandatory": True,
        },
    ),
    (
        "in_fod",
        ImageIn,
        {
            "argstr": "",
            "position": 1,
            "help_string": """input image containing the spherical harmonics of the fibre orientation distributions""",
            "mandatory": True,
        },
    ),
    (
        "out_tracks",
        Tracks,
        {
            "argstr": "",
            "position": 2,
            "help_string": """the output filtered tracks file""",
            "mandatory": True,
        },
    ),
    (
        "nofilter",
        bool,
        {
            "argstr": "-nofilter",
            "help_string": """do NOT perform track filtering - just construct the model in order to provide output debugging images""",
        },
    ),
    (
        "output_at_counts",
        ty.List[int],
        {
            "argstr": "-output_at_counts",
            "help_string": """output filtered track files (and optionally debugging images if -output_debug is specified) at specific numbers of remaining streamlines; provide as comma-separated list of integers""",
            "sep": ",",
        },
    ),
    # Options for setting the processing mask for the SIFT fixel-streamlines comparison model Option Group
    (
        "proc_mask",
        ImageIn,
        {
            "argstr": "-proc_mask",
            "help_string": """provide an image containing the processing mask weights for the model; image spatial dimensions must match the fixel image""",
        },
    ),
    (
        "act",
        ImageIn,
        {
            "argstr": "-act",
            "help_string": """use an ACT five-tissue-type segmented anatomical image to derive the processing mask""",
        },
    ),
    # Options affecting the SIFT model Option Group
    (
        "fd_scale_gm",
        bool,
        {
            "argstr": "-fd_scale_gm",
            "help_string": """provide this option (in conjunction with -act) to heuristically downsize the fibre density estimates based on the presence of GM in the voxel. This can assist in reducing tissue interface effects when using a single-tissue deconvolution algorithm""",
        },
    ),
    (
        "no_dilate_lut",
        bool,
        {
            "argstr": "-no_dilate_lut",
            "help_string": """do NOT dilate FOD lobe lookup tables; only map streamlines to FOD lobes if the precise tangent lies within the angular spread of that lobe""",
        },
    ),
    (
        "make_null_lobes",
        bool,
        {
            "argstr": "-make_null_lobes",
            "help_string": """add an additional FOD lobe to each voxel, with zero integral, that covers all directions with zero / negative FOD amplitudes""",
        },
    ),
    (
        "remove_untracked",
        bool,
        {
            "argstr": "-remove_untracked",
            "help_string": """remove FOD lobes that do not have any streamline density attributed to them; this improves filtering slightly, at the expense of longer computation time (and you can no longer do quantitative comparisons between reconstructions if this is enabled)""",
        },
    ),
    (
        "fd_thresh",
        float,
        {
            "argstr": "-fd_thresh",
            "help_string": """fibre density threshold; exclude an FOD lobe from filtering processing if its integral is less than this amount (streamlines will still be mapped to it, but it will not contribute to the cost function or the filtering)""",
        },
    ),
    # Options to make SIFT provide additional output files Option Group
    (
        "csv",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-csv",
            "output_file_template": "csv.txt",
            "help_string": """output statistics of execution per iteration to a .csv file""",
        },
    ),
    (
        "out_mu",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-out_mu",
            "output_file_template": "out_mu.txt",
            "help_string": """output the final value of SIFT proportionality coefficient mu to a text file""",
        },
    ),
    (
        "output_debug",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-output_debug",
            "output_file_template": "output_debug",
            "help_string": """write to a directory various output images for assessing & debugging performance etc.""",
        },
    ),
    (
        "out_selection",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-out_selection",
            "output_file_template": "out_selection.txt",
            "help_string": """output a text file containing the binary selection of streamlines""",
        },
    ),
    # Options to control when SIFT terminates filtering Option Group
    (
        "term_number",
        int,
        {
            "argstr": "-term_number",
            "help_string": """number of streamlines - continue filtering until this number of streamlines remain""",
        },
    ),
    (
        "term_ratio",
        float,
        {
            "argstr": "-term_ratio",
            "help_string": """termination ratio - defined as the ratio between reduction in cost function, and reduction in density of streamlines.
Smaller values result in more streamlines being filtered out.""",
        },
    ),
    (
        "term_mu",
        float,
        {
            "argstr": "-term_mu",
            "help_string": """terminate filtering once the SIFT proportionality coefficient reaches a given value""",
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

tcksift_input_spec = specs.SpecInfo(
    name="tcksift_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "csv",
        File,
        {
            "help_string": """output statistics of execution per iteration to a .csv file""",
        },
    ),
    (
        "out_mu",
        File,
        {
            "help_string": """output the final value of SIFT proportionality coefficient mu to a text file""",
        },
    ),
    (
        "output_debug",
        Directory,
        {
            "help_string": """write to a directory various output images for assessing & debugging performance etc.""",
        },
    ),
    (
        "out_selection",
        File,
        {
            "help_string": """output a text file containing the binary selection of streamlines""",
        },
    ),
]
tcksift_output_spec = specs.SpecInfo(
    name="tcksift_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class tcksift(ShellCommandTask):
    """
        References
        ----------

            Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. SIFT: Spherical-deconvolution informed filtering of tractograms. NeuroImage, 2013, 67, 298-312

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

    executable = "tcksift"
    input_spec = tcksift_input_spec
    output_spec = tcksift_output_spec
