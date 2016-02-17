tckstats
===========

Synopsis
--------

::

    tckstats [ options ]  tracks_in

-  *tracks_in*: the input track file

Description
-----------

calculate statistics on streamlines length.

Options
-------

-  **-histogram path** output a histogram of streamline lengths

-  **-dump path** dump the streamlines lengths to a text file

-  **-tck_weights_in path** specify a text scalar file containing the
   streamline weights

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
