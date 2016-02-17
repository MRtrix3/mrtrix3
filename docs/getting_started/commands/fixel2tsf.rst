fixel2tsf
===========

Synopsis
--------

::

    fixel2tsf [ options ]  fixel_in tracks tsf

-  *fixel_in*: the input fixel image
-  *tracks*: the input track file
-  *tsf*: the output track scalar file

Description
-----------

Map fixel values to a track scalar file based on an input tractogram.
This is useful for visualising the output from fixelcfestats in 3D.

Options
-------

-  **-angle value** the max anglular threshold for computing
   correspondence between a fixel direction and track tangent

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

**Author:** David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (C) 2008 Brain Research Institute, Melbourne,
Australia. This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.
