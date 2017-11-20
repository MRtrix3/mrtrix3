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
-  *assignments_in*: text file containing the node assignments for each streamline
-  *prefix_out*: the output file / prefix

Description
-----------

The compulsory input file "assignments_in" should contain a text file where there is one row for each streamline, and each row contains a list of numbers corresponding to the parcels to which that streamline was assigned (most typically there will be two entries per streamline, one for each endpoint; but this is not strictly a requirement). This file will most typically be generated using the tck2connectome command with the -out_assignments option.

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

-  **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

-  **-failonwarn** terminate program if a warning is produced

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


