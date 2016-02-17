shconv
===========

Synopsis
--------

::

    shconv [ options ]  SH response SH

-  *SH*: the input spherical harmonics coefficients image.
-  *response*: the convolution kernel (response function)
-  *SH*: the output spherical harmonics coefficients image.

Description
-----------

perform a spherical convolution

Options
-------

-  **-mask image** only perform computation within the specified binary
   brain mask image.

Stride options
^^^^^^^^^^^^^^

-  **-stride spec** specify the strides of the output data in memory,
   as a comma-separated list. The actual strides produced will depend on
   whether the output image format can support it.

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
