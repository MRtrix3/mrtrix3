connectome2tck
===========

Synopsis
--------

::

    connectome2tck [ options ]  tracks_in assignments_in prefix_out

-  *tracks_in*: the input track file
-  *assignments_in*: text file containing the node assignments for each
   streamline
-  *prefix_out*: the output file / prefix

Description
-----------

extract streamlines from a tractogram based on their assignment to
parcellated nodes

Options
-------

Options for determining the content / format of output files
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-nodes list** only select tracks that involve a set of nodes of
   interest (provide as a comma-separated list of integers)

-  **-exclusive** only select tracks that exclusively connect nodes
   from within the list of nodes of interest

-  **-files option** select how the resulting streamlines will be
   grouped in output files. Options are: per_edge, per_node, single
   (default: per_edge)

-  **-exemplars image** generate a mean connection exemplar per edge,
   rather than keeping all streamlines (the parcellation node image must
   be provided in order to constrain the exemplar endpoints)

-  **-keep_unassigned** by default, the program discards those
   streamlines that are not successfully assigned to a node. Set this
   option to generate corresponding outputs containing these streamlines
   (labelled as node index 0)

Options for importing / exporting streamline weights
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-tck_weights_in path** specify a text scalar file containing the
   streamline weights

-  **-prefix_tck_weights_out prefix** provide a prefix for
   outputting a text file corresponding to each output file, each
   containing only the streamline weights relevant for that track file

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
