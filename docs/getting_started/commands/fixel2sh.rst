fixel2sh
===========

Synopsis
--------

::

    fixel2sh [ options ]  fixel_in sh_out

-  *fixel_in*: the input sparse fixel image.
-  *sh_out*: the output sh image.

Description
-----------

convert a fixel-based sparse-data image into an SH image that can be
visually evaluated using MRview

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

**Author:** Robert E. Smith (r.smith@brain.org.au)

**Copyright:** Copyright (C) 2008 Brain Research Institute, Melbourne,
Australia. This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.
