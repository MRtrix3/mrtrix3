mrcrop
===========

Synopsis
--------

::

    mrcrop [ options ]  image_in image_out

-  *image_in*: the image to be cropped
-  *image_out*: the output path for the resulting cropped image

Description
-----------

Crop an image series to a reduced field of view, using either manual
setting of axis dimensions, or a computed mask image corresponding to
the brain.

If using a mask, a gap of 1 voxel will be left at all 6 edges of the
image such that trilinear interpolation upon the resulting images is
still valid.

This is useful for axially-acquired brain images, where the image size
can be reduced by a factor of 2 by removing the empty space on either
side of the brain.

Options
-------

-  **-mask image** crop the input image according to the spatial extent
   of a mask image

-  **-axis index start end** crop the input image in the provided axis

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
