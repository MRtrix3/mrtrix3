dirflip
===========

Synopsis
--------

::

    dirflip [ options ]  in out

-  *in*: the input files for the directions.
-  *out*: the output files for the directions.

Description
-----------

optimise the polarity of the directions in a scheme with respect to a
unipolar electrostatic repulsion model, by inversion of individual
directions. The orientations themselves are not affected, only their
polarity. This is necessary to ensure near-optimal distribution of DW
directions for eddy-current correction.

Options
-------

-  **-permutations num** number of permutations to try

-  **-cartesian** Output the directions in Cartesian coordinates [x y
   z] instead of [az el].

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
