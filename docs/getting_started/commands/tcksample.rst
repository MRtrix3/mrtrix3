.. _tcksample:

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

The values are written to the output file as ASCII text, in the same order as the track vertices, with all values for each track on the same line (space separated), and individual tracks on separate lines.

Options
-------

Streamline resampling options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-resample num start end** resample tracks before sampling from the image, by resampling the tracks at 'num' equidistant and comparable locations along the tracks between 'start' and 'end' (specified as comma-separated 3-vectors in scanner coordinates)

-  **-waypoint point** [only used with -resample] together with the start and end points, this defines an arc of a circle passing through all points, along which resampling is to occur.

-  **-locations file** [only used with -resample] output a new track file with vertices at the locations resampled by the algorithm.

-  **-warp image** [only used with -resample] specify an image containing the warp field to the space in which the resampling is to take place. The tracks will be resampled as per their locations in the warped space, with sampling itself taking place in the original space

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

-  **-failonwarn** terminate program if a warning is produced

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------



**Author:** J-Donald Tournier (jdtournier@gmail.com)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

