sh2response
===========

Synopsis
--------

::

    sh2response [ options ]  SH mask directions response

-  *SH*: the spherical harmonic decomposition of the diffusion-weighted
   images
-  *mask*: the mask containing the voxels from which to estimate the
   response function
-  *directions*: a 4D image containing the direction vectors along which
   to estimate the response function
-  *response*: the output axially-symmetric spherical harmonic
   coefficients

Description
-----------

generate an appropriate response function from the image data for
spherical deconvolution

Options
-------

-  **-lmax value** specify the maximum harmonic degree of the response
   function to estimate

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
