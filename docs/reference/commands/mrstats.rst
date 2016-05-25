.. _mrstats:

mrstats
===========

Synopsis
--------

::

    mrstats [ options ]  image

-  *image*: the input image from which statistics will be computed.

Description
-----------

compute images statistics.

Options
-------

Statistics options
^^^^^^^^^^^^^^^^^^

-  **-output field** output only the field specified. Multiple such options can be supplied if required. Choices are: mean, median, std, min, max, count. Useful for use in scripts.

-  **-mask image** only perform computation within the specified binary mask image.

-  **-histogram file** generate histogram of intensities and store in specified text file. Note that the first line of the histogram gives the centre of the bins.

-  **-bins num** the number of bins to use to generate the histogram (default = 100).

-  **-dump file** dump the voxel intensities to a text file.

-  **-voxel pos** only perform computation within the specified voxel, supplied as a comma-separated vector of 3 integer values (multiple voxels can be included).

-  **-position file** dump the position of the voxels in the mask to a text file.

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

