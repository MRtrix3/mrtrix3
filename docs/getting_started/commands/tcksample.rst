tcksample
===========

Synopsis
--------

::

    tcksample [ options ]  tracks image values

-  *tracks*: the input track file
-  *image*: the image to be sampled
-  *values*: the output sampled values

Description
-----------

sample values of associated image at each location along tracks

The values are written to the output file as ASCII text, in the same
order as the track vertices, with all values for each track on the same
line (space separated), and individual tracks on separate lines.

Options
-------

Streamline resampling options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-resample num start end** resample tracks before sampling from the
   image, by resampling the tracks at 'num' equidistant and comparable
   locations along the tracks between 'start' and 'end' (specified as
   comma-separated 3-vectors in scanner coordinates)

-  **-waypoint point** [only used with -resample] together with the
   start and end points, this defines an arc of a circle passing through
   all points, along which resampling is to occur.

-  **-locations file** [only used with -resample] output a new track
   file with vertices at the locations resampled by the algorithm.

-  **-warp image** [only used with -resample] specify an image
   containing the warp field to the space in which the resampling is to
   take place. The tracks will be resampled as per their locations in
   the warped space, with sampling itself taking place in the original
   space

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
