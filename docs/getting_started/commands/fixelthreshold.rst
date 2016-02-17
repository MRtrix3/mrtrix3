fixelthreshold
===========

Synopsis
--------

::

    fixelthreshold [ options ]  fixel_in threshold fixel_out

-  *fixel_in*: the input fixel image.
-  *threshold*: the input threshold
-  *fixel_out*: the output fixel image

Description
-----------

Threshold the values in a fixel image

Options
-------

-  **-crop** remove fixels that fall below threshold (instead of
   assigning their value to zero or one)

-  **-invert** invert the output image (i.e. below threshold fixels are
   included instead)

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
