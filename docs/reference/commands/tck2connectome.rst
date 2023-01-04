.. _tck2connectome:

tck2connectome
===================

Synopsis
--------

Generate a connectome matrix from a streamlines file and a node parcellation image

Usage
--------

::

    tck2connectome [ options ]  tracks_in nodes_in connectome_out

-  *tracks_in*: the input track file
-  *nodes_in*: the input node parcellation image
-  *connectome_out*: the output .csv file containing edge weights

Example usages
--------------

-   *Default usage*::

        $ tck2connectome tracks.tck nodes.mif connectome.csv -tck_weights_in weights.csv -out_assignments assignments.txt

    By default, the metric of connectivity quantified in the connectome matrix is the number of streamlines; or, if tcksift2 is used, the sum of streamline weights via the -tck_weights_in option. Use of the -out_assignments option is recommended as this enables subsequent use of the connectome2tck command.

-   *Generate a matrix consisting of the mean streamline length between each node pair*::

        $ tck2connectome tracks.tck nodes.mif distances.csv -scale_length -stat_edge mean

    By multiplying the contribution of each streamline to the connectome by the length of that streamline, and then, for each edge, computing the mean value across the contributing streamlines, one obtains a matrix where the value in each entry is the mean length across those streamlines belonging to that edge.

-   *Generate a connectome matrix where the value of connectivity is the "mean FA"*::

        $ tcksample tracks.tck FA.mif mean_FA_per_streamline.csv -stat_tck mean; tck2connectome tracks.tck nodes.mif mean_FA_connectome.csv -scale_file mean_FA_per_streamline.csv -stat_edge mean

    Here, a connectome matrix that is "weighted by FA" is generated in multiple steps: firstly, for each streamline, the value of the underlying FA image is sampled at each vertex, and the mean of these values is calculated to produce a single scalar value of "mean FA" per streamline; then, as each streamline is assigned to nodes within the connectome, the magnitude of the contribution of that streamline to the matrix is multiplied by the mean FA value calculated prior for that streamline; finally, for each connectome edge, across the values of "mean FA" that were contributed by all of the streamlines assigned to that particular edge, the mean value is calculated.

-   *Generate the connectivity fingerprint for streamlines seeded from a particular region*::

        $ tck2connectome fixed_seed_tracks.tck nodes.mif fingerprint.csv -vector

    This usage assumes that the streamlines being provided to the command have all been seeded from the (effectively) same location, and as such, only the endpoint of each streamline (not their starting point) is assigned based on the provided parcellation image. Accordingly, the output file contains only a vector of connectivity values rather than a matrix, since each streamline is assigned to only one node rather than two.

Options
-------

Structural connectome streamline assignment option
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-assignment_end_voxels** use a simple voxel lookup value at each streamline endpoint

-  **-assignment_radial_search radius** perform a radial search from each streamline endpoint to locate the nearest node. Argument is the maximum radius in mm; if no node is found within this radius, the streamline endpoint is not assigned to any node. Default search distance is 4mm.

-  **-assignment_reverse_search max_dist** traverse from each streamline endpoint inwards along the streamline, in search of the last node traversed by the streamline. Argument is the maximum traversal length in mm (set to 0 to allow search to continue to the streamline midpoint).

-  **-assignment_forward_search max_dist** project the streamline forwards from the endpoint in search of a parcellation node voxel. Argument is the maximum traversal length in mm.

-  **-assignment_all_voxels** assign the streamline to all nodes it intersects along its length (note that this means a streamline may be assigned to more than two nodes, or indeed none at all)

Structural connectome metric options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-scale_length** scale each contribution to the connectome edge by the length of the streamline

-  **-scale_invlength** scale each contribution to the connectome edge by the inverse of the streamline length

-  **-scale_invnodevol** scale each contribution to the connectome edge by the inverse of the two node volumes

-  **-scale_file path** scale each contribution to the connectome edge according to the values in a vector file

Options for outputting connectome matrices
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-symmetric** Make matrices symmetric on output

-  **-zero_diagonal** Set matrix diagonal to zero on output

Other options for tck2connectome
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-stat_edge statistic** statistic for combining the values from all streamlines in an edge into a single scale value for that edge (options are: sum,mean,min,max; default=sum)

-  **-tck_weights_in path** specify a text scalar file containing the streamline weights

-  **-keep_unassigned** By default, the program discards the information regarding those streamlines that are not successfully assigned to a node pair. Set this option to keep these values (will be the first row/column in the output matrix)

-  **-out_assignments path** output the node assignments of each streamline to a file; this can be used subsequently e.g. by the command connectome2tck

-  **-vector** output a vector representing connectivities from a given seed point to target nodes, rather than a matrix of node-node connectivities

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status; alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files (caution: using the same file as input and output might cause unexpected behaviour).

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-config key value** *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

If using the default streamline-parcel assignment mechanism (or -assignment_radial_search option): Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. The effects of SIFT on the reproducibility and biological accuracy of the structural connectome. NeuroImage, 2015, 104, 253-265

If using -scale_invlength or -scale_invnodevol options: Hagmann, P.; Cammoun, L.; Gigandet, X.; Meuli, R.; Honey, C.; Wedeen, V. & Sporns, O. Mapping the Structural Core of Human Cerebral Cortex. PLoS Biology 6(7), e159

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2023 the MRtrix3 contributors.

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


