dirmerge
===========

Synopsis
--------

::

    dirmerge [ options ]  subsets bvalue files [ bvalue files ... ] out

-  *subsets*: the number of subsets (phase-encode directions) per
   b-value
-  *bvalue files*: the b-value and sets of corresponding files, in order
-  *out*: the output directions file, with each row listing the X Y Z
   gradient directions, the b-value, and an index representing the phase
   encode direction

Description
-----------

splice or merge sets of directions over multiple shells into a single
set, in such a way as to maintain near-optimality upon truncation.

Options
-------

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

**Author:** J-Donald Tournier (jdtournier@gmail.com)

**Copyright:** Copyright (C) 2008 Brain Research Institute, Melbourne,
Australia. This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.
