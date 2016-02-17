tck2connectome
===========

Synopsis
--------

::

    tck2connectome [ options ]  tracks_in nodes_in connectome_out

-  *tracks_in*: the input track file
-  *nodes_in*: the input node parcellation image
-  *connectome_out*: the output .csv file containing edge weights

Description
-----------

generate a connectome matrix from a streamlines file and a node
parcellation image

Options
-------

Structural connectome streamline assignment option
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-assignment_end_voxels** use a simple voxel lookup value at each
   streamline endpoint

-  **-assignment_radial_search radius** perform a radial search from
   each streamline endpoint to locate the nearest node.Argument is the
   maximum radius in mm; if no node is found within this radius, the
   streamline endpoint is not assigned to any node.

-  **-assignment_reverse_search max_dist** traverse from each
   streamline endpoint inwards along the streamline, in search of the
   last node traversed by the streamline. Argument is the maximum
   traversal length in mm (set to 0 to allow search to continue to the
   streamline midpoint).

-  **-assignment_forward_search max_dist** project the streamline
   forwards from the endpoint in search of a parcellation node voxel.
   Argument is the maximum traversal length in mm.

-  **-assignment_all_voxels** assign the streamline to all nodes it
   intersects along its length (note that this means a streamline may be
   assigned to more than two nodes, or indeed none at all)

Structural connectome metric option
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-metric choice** specify the edge weight metric. Options are:
   count (default), meanlength, invlength, invnodevolume,
   invlength_invnodevolume, mean_scalar

-  **-image path** provide the associated image for the mean_scalar
   metric

-  **-tck_weights_in path** specify a text scalar file containing the
   streamline weights

-  **-keep_unassigned** By default, the program discards the
   information regarding those streamlines that are not successfully
   assigned to a node pair. Set this option to keep these values (will
   be the first row/column in the output matrix)

-  **-out_assignments path** output the node assignments of each
   streamline to a file

-  **-zero_diagonal** set all diagonal entries in the matrix to zero
   (these represent streamlines that connect to the same node at both
   ends)

-  **-vector** output a vector representing connectivities from a given
   seed point to target nodes, rather than a matrix of node-node
   connectivities

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same
   file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded
   applications

-  **-failonwarn** terminate program if a warning is produced

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------

MRtrix new_syntax-1436-ge228c30b, built Feb 17 2016

**Author:** Robert E. Smith (r.smith@brain.org.au)

**Copyright:** Copyright (C) 2008 Brain Research Institute, Melbourne,
Australia. This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.
