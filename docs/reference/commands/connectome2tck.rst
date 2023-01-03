.. _connectome2tck:

connectome2tck
===================

Synopsis
--------

Extract streamlines from a tractogram based on their assignment to parcellated nodes

Usage
--------

::

    connectome2tck [ options ]  tracks_in assignments_in prefix_out

-  *tracks_in*: the input track file
-  *assignments_in*: input text file containing the node assignments for each streamline
-  *prefix_out*: the output file / prefix

Description
-----------

The compulsory input file "assignments_in" should contain a text file where there is one row for each streamline, and each row contains a list of numbers corresponding to the parcels to which that streamline was assigned (most typically there will be two entries per streamline, one for each endpoint; but this is not strictly a requirement). This file will most typically be generated using the tck2connectome command with the -out_assignments option.

Example usages
--------------

-   *Default usage*::

        $ connectome2tck tracks.tck assignments.txt edge-

    The command will generate one track file for every edge in the connectome, with the name of each file indicating the nodes connected via that edge; for instance, all streamlines connecting nodes 23 and 49 will be written to file "edge-23-49.tck".

-   *Extract only the streamlines between nodes 1 and 2*::

        $ connectome2tck tracks.tck assignments.txt tracks_1_2.tck -nodes 1,2 -exclusive -files single

    Since only a single edge is of interest, this example provides only the two nodes involved in that edge to the -nodes option, adds the -exclusive option so that only streamlines for which both assigned nodes are in the list of nodes of interest are extracted (i.e. only streamlines connecting nodes 1 and 2 in this example), and writes the result to a single output track file.

-   *Extract the streamlines connecting node 15 to all other nodes in the parcellation, with one track file for each edge*::

        $ connectome2tck tracks.tck assignments.txt from_15_to_ -nodes 15 -keep_self

    The command will generate the same number of track files as there are nodes in the parcellation: one each for the streamlines connecting node 15 to every other node; i.e. "from_15_to_1.tck", "from_15_to_2.tck", "from_15_to_3.tck", etc.. Because the -keep_self option is specified, file "from_15_to_15.tck" will also be generated, containing those streamlines that connect to node 15 at both endpoints.

-   *For every node, generate a file containing all streamlines connected to that node*::

        $ connectome2tck tracks.tck assignments.txt node -files per_node

    Here the command will generate one track file for every node in the connectome: "node1.tck", "node2.tck", "node3.tck", etc.. Each of these files will contain all streamlines that connect the node of that index to another node in the connectome (it does not select all tracks connecting a particular node, since the -keep_self option was omitted and therefore e.g. a streamline that is assigned to node 41 will not be present in file "node41.tck"). Each streamline in the input tractogram will in fact appear in two different output track files; e.g. a streamline connecting nodes 8 and 56 will be present both in file "node8.tck" and file "node56.tck".

-   *Get all streamlines that were not successfully assigned to a node pair*::

        $ connectome2tck tracks.tck assignments.txt unassigned.tck -nodes 0 -keep_self -files single

    Node index 0 corresponds to streamline endpoints that were not successfully assigned to a node. As such, by selecting all streamlines that are assigned to "node 0" (including those streamlines for which neither endpoint is assigned to a node due to use of the -keep_self option), the single output track file will contain all streamlines for which at least one of the two endpoints was not successfully assigned to a node.

-   *Generate a single track file containing edge exemplar trajectories*::

        $ connectome2tck tracks.tck assignments.txt exemplars.tck -files single -exemplars nodes.mif

    This produces the track file that is required as input when attempting to display connectome edges using the streamlines or streamtubes geometries within the meview connectome tool.

Options
-------

Options for determining the content / format of output files
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-nodes list** only select tracks that involve a set of nodes of interest (provide as a comma-separated list of integers)

-  **-exclusive** only select tracks that exclusively connect nodes from within the list of nodes of interest

-  **-files option** select how the resulting streamlines will be grouped in output files. Options are: per_edge, per_node, single (default: per_edge)

-  **-exemplars image** generate a mean connection exemplar per edge, rather than keeping all streamlines (the parcellation node image must be provided in order to constrain the exemplar endpoints)

-  **-keep_unassigned** by default, the program discards those streamlines that are not successfully assigned to a node. Set this option to generate corresponding outputs containing these streamlines (labelled as node index 0)

-  **-keep_self** by default, the program will not output streamlines that connect to the same node at both ends. Set this option to instead keep these self-connections.

Options for importing / exporting streamline weights
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-tck_weights_in path** specify a text scalar file containing the streamline weights

-  **-prefix_tck_weights_out prefix** provide a prefix for outputting a text file corresponding to each output file, each containing only the streamline weights relevant for that track file

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


