mrmesh
===========

Synopsis
--------

::

    mrmesh [ options ]  input output

-  *input*: the input image.
-  *output*: the output mesh file.

Description
-----------

Generate a mesh file from an image.

Options
-------

-  **-blocky** generate a 'blocky' mesh that precisely represents the
   voxel edges

-  **-threshold value** manually set the intensity threshold at which
   the mesh will be generated (if omitted, a threshold will be
   determined automatically)

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
