.. _maskfilter:

maskfilter
===================

Synopsis
--------

Perform filtering operations on 3D / 4D mask images

Usage
--------

::

    maskfilter [ options ]  input filter output

-  *input*: the input image.
-  *filter*: the type of filter to be applied (clean, connect, dilate, erode, median)
-  *output*: the output image.

Description
-----------

The available filters are: clean, connect, dilate, erode, median.

Each filter has its own unique set of optional parameters.

Options
-------

Options for mask cleaning filter
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-scale value** the maximum scale used to cut bridges. A certain maximum scale cuts bridges up to a width (in voxels) of 2x the provided scale. (Default: 2)

Options for connected-component filter
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-axes axes** specify which axes should be included in the connected components. By default only the first 3 axes are included. The axes should be provided as a comma-separated list of values.

-  **-largest** only retain the largest connected component

-  **-connectivity** use 26-voxel-neighbourhood connectivity (Default: 6)

Options for dilate / erode filters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-npass value** the number of times to repeatedly apply the filter

Options for median filter
^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-extent voxels** specify the extent (width) of kernel size in voxels. This can be specified either as a single value to be used for all axes, or as a comma-separated list of the extent for each axis. The default is 3x3x3.

Stride options
^^^^^^^^^^^^^^

-  **-stride spec** specify the strides of the output data in memory, as a comma-separated list. The actual strides produced will depend on whether the output image format can support it.

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



**Author:** Robert E. Smith (robert.smith@florey.edu.au), David Raffelt (david.raffelt@florey.edu.au), Thijs Dhollander (thijs.dhollander@gmail.com) and J-Donald Tournier (jdtournier@gmail.com)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.

