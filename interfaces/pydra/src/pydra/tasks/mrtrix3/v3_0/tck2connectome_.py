# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "tracks_in",
        Tracks,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the input track file""",
            "mandatory": True,
        },
    ),
    (
        "nodes_in",
        ImageIn,
        {
            "argstr": "",
            "position": 1,
            "help_string": """the input node parcellation image""",
            "mandatory": True,
        },
    ),
    (
        "connectome_out",
        Path,
        {
            "argstr": "",
            "position": 2,
            "output_file_template": "connectome_out.txt",
            "help_string": """the output .csv file containing edge weights""",
        },
    ),
    # Structural connectome streamline assignment option Option Group
    (
        "assignment_end_voxels",
        bool,
        {
            "argstr": "-assignment_end_voxels",
            "help_string": """use a simple voxel lookup value at each streamline endpoint""",
        },
    ),
    (
        "assignment_radial_search",
        float,
        {
            "argstr": "-assignment_radial_search",
            "help_string": """perform a radial search from each streamline endpoint to locate the nearest node. Argument is the maximum radius in mm; if no node is found within this radius, the streamline endpoint is not assigned to any node. Default search distance is 4mm.""",
        },
    ),
    (
        "assignment_reverse_search",
        float,
        {
            "argstr": "-assignment_reverse_search",
            "help_string": """traverse from each streamline endpoint inwards along the streamline, in search of the last node traversed by the streamline. Argument is the maximum traversal length in mm (set to 0 to allow search to continue to the streamline midpoint).""",
        },
    ),
    (
        "assignment_forward_search",
        float,
        {
            "argstr": "-assignment_forward_search",
            "help_string": """project the streamline forwards from the endpoint in search of a parcellation node voxel. Argument is the maximum traversal length in mm.""",
        },
    ),
    (
        "assignment_all_voxels",
        bool,
        {
            "argstr": "-assignment_all_voxels",
            "help_string": """assign the streamline to all nodes it intersects along its length (note that this means a streamline may be assigned to more than two nodes, or indeed none at all)""",
        },
    ),
    # Structural connectome metric options Option Group
    (
        "scale_length",
        bool,
        {
            "argstr": "-scale_length",
            "help_string": """scale each contribution to the connectome edge by the length of the streamline""",
        },
    ),
    (
        "scale_invlength",
        bool,
        {
            "argstr": "-scale_invlength",
            "help_string": """scale each contribution to the connectome edge by the inverse of the streamline length""",
        },
    ),
    (
        "scale_invnodevol",
        bool,
        {
            "argstr": "-scale_invnodevol",
            "help_string": """scale each contribution to the connectome edge by the inverse of the two node volumes""",
        },
    ),
    (
        "scale_file",
        ImageIn,
        {
            "argstr": "-scale_file",
            "help_string": """scale each contribution to the connectome edge according to the values in a vector file""",
        },
    ),
    # Options for outputting connectome matrices Option Group
    (
        "symmetric",
        bool,
        {
            "argstr": "-symmetric",
            "help_string": """Make matrices symmetric on output""",
        },
    ),
    (
        "zero_diagonal",
        bool,
        {
            "argstr": "-zero_diagonal",
            "help_string": """Set matrix diagonal to zero on output""",
        },
    ),
    # Other options for tck2connectome Option Group
    (
        "stat_edge",
        str,
        {
            "argstr": "-stat_edge",
            "help_string": """statistic for combining the values from all streamlines in an edge into a single scale value for that edge (options are: sum,mean,min,max; default=sum)""",
            "allowed_values": ["sum", "sum", "mean", "min", "max"],
        },
    ),
    (
        "tck_weights_in",
        File,
        {
            "argstr": "-tck_weights_in",
            "help_string": """specify a text scalar file containing the streamline weights""",
        },
    ),
    (
        "keep_unassigned",
        bool,
        {
            "argstr": "-keep_unassigned",
            "help_string": """By default, the program discards the information regarding those streamlines that are not successfully assigned to a node pair. Set this option to keep these values (will be the first row/column in the output matrix)""",
        },
    ),
    (
        "out_assignments",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-out_assignments",
            "output_file_template": "out_assignments.txt",
            "help_string": """output the node assignments of each streamline to a file; this can be used subsequently e.g. by the command connectome2tck""",
        },
    ),
    (
        "vector",
        bool,
        {
            "argstr": "-vector",
            "help_string": """output a vector representing connectivities from a given seed point to target nodes, rather than a matrix of node-node connectivities""",
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

tck2connectome_input_spec = specs.SpecInfo(
    name="tck2connectome_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "connectome_out",
        File,
        {
            "help_string": """the output .csv file containing edge weights""",
        },
    ),
    (
        "out_assignments",
        File,
        {
            "help_string": """output the node assignments of each streamline to a file; this can be used subsequently e.g. by the command connectome2tck""",
        },
    ),
]
tck2connectome_output_spec = specs.SpecInfo(
    name="tck2connectome_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class tck2connectome(ShellCommandTask):
    """
        Example usages
        --------------


        Default usage:

            `$ tck2connectome tracks.tck nodes.mif connectome.csv -tck_weights_in weights.csv -out_assignments assignments.txt`

            By default, the metric of connectivity quantified in the connectome matrix is the number of streamlines; or, if tcksift2 is used, the sum of streamline weights via the -tck_weights_in option. Use of the -out_assignments option is recommended as this enables subsequent use of the connectome2tck command.


        Generate a matrix consisting of the mean streamline length between each node pair:

            `$ tck2connectome tracks.tck nodes.mif distances.csv -scale_length -stat_edge mean`

            By multiplying the contribution of each streamline to the connectome by the length of that streamline, and then, for each edge, computing the mean value across the contributing streamlines, one obtains a matrix where the value in each entry is the mean length across those streamlines belonging to that edge.


        Generate a connectome matrix where the value of connectivity is the "mean FA":

            `$ tcksample tracks.tck FA.mif mean_FA_per_streamline.csv -stat_tck mean; tck2connectome tracks.tck nodes.mif mean_FA_connectome.csv -scale_file mean_FA_per_streamline.csv -stat_edge mean`

            Here, a connectome matrix that is "weighted by FA" is generated in multiple steps: firstly, for each streamline, the value of the underlying FA image is sampled at each vertex, and the mean of these values is calculated to produce a single scalar value of "mean FA" per streamline; then, as each streamline is assigned to nodes within the connectome, the magnitude of the contribution of that streamline to the matrix is multiplied by the mean FA value calculated prior for that streamline; finally, for each connectome edge, across the values of "mean FA" that were contributed by all of the streamlines assigned to that particular edge, the mean value is calculated.


        Generate the connectivity fingerprint for streamlines seeded from a particular region:

            `$ tck2connectome fixed_seed_tracks.tck nodes.mif fingerprint.csv -vector`

            This usage assumes that the streamlines being provided to the command have all been seeded from the (effectively) same location, and as such, only the endpoint of each streamline (not their starting point) is assigned based on the provided parcellation image. Accordingly, the output file contains only a vector of connectivity values rather than a matrix, since each streamline is assigned to only one node rather than two.


        References
        ----------

            If using the default streamline-parcel assignment mechanism (or -assignment_radial_search option): Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. The effects of SIFT on the reproducibility and biological accuracy of the structural connectome. NeuroImage, 2015, 104, 253-265

            If using -scale_invlength or -scale_invnodevol options: Hagmann, P.; Cammoun, L.; Gigandet, X.; Meuli, R.; Honey, C.; Wedeen, V. & Sporns, O. Mapping the Structural Core of Human Cerebral Cortex. PLoS Biology 6(7), e159

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

    executable = "tck2connectome"
    input_spec = tck2connectome_input_spec
    output_spec = tck2connectome_output_spec
