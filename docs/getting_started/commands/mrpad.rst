mrpad
===========

Synopsis
--------

::

    mrpad [ options ]  image_in image_out

-  *image_in*: the image to be padded
-  *image_out*: the output path for the resulting padded image

Description
-----------

Pad an image to increase the FOV

Options
-------

-  **-uniform number** pad the input image by a uniform number of
   voxels on all sides (in 3D)

-  **-axis index lower upper** pad the input image along the provided
   axis (defined by index). Lower and upper define the number of voxels
   to add to the lower and upper bounds of the axis

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
