sh2peaks
===========

Synopsis
--------

::

    sh2peaks [ options ]  SH output

-  *SH*: the input image of SH coefficients.
-  *output*: the output image. Each volume corresponds to the x, y & z
   component of each peak direction vector in turn.

Description
-----------

extract the peaks of a spherical harmonic function at each voxel, by
commencing a Newton search along a set of specified directions

Options
-------

-  **-num peaks** the number of peaks to extract (default is 3).

-  **-direction phi theta** the direction of a peak to estimate. The
   algorithm will attempt to find the same number of peaks as have been
   specified using this option.

-  **-peaks image** the program will try to find the peaks that most
   closely match those in the image provided.

-  **-threshold value** only peak amplitudes greater than the threshold
   will be considered.

-  **-seeds file** specify a set of directions from which to start the
   multiple restarts of the optimisation (by default, the built-in 60
   direction set is used)

-  **-mask image** only perform computation within the specified binary
   brain mask image.

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
