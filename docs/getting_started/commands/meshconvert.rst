meshconvert
===========

Synopsis
--------

::

    meshconvert [ options ]  input output

-  *input*: the input mesh file
-  *output*: the output mesh file

Description
-----------

convert meshes between different formats, and apply transformations.

Options
-------

-  **-binary** write the output file in binary format

Options for applying spatial transformations to vertices
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-transform_first2real image** transform vertices from FSL FIRST's
   native corrdinate space to real space

-  **-transform_real2first image** transform vertices from FSL real
   space to FIRST's native corrdinate space

-  **-transform_voxel2real image** transform vertices from voxel space
   to real space

-  **-transform_real2voxel image** transform vertices from real space
   to voxel space

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
