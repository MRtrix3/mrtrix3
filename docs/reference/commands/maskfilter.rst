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

-  **-strides spec** specify the strides of the output data in memory; either as a comma-separated list of (signed) integers, or as a template image from which the strides shall be extracted and used. The actual strides produced will depend on whether the output image format can support it.

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status; alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files (caution: using the same file as input and output might cause unexpected behaviour).

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-config key value** *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au), David Raffelt (david.raffelt@florey.edu.au), Thijs Dhollander (thijs.dhollander@gmail.com) and J-Donald Tournier (jdtournier@gmail.com)

**Copyright:** Copyright (c) 2008-2023 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Covered Software is provided under this License on an "as is"
basis, without warranty of any kind, either expressed, implied, or
statutory, including, without limitation, warranties that the
Covered Software is free of defects, merchantable, fit for a
particular purpose or non-infringing.
See the Mozilla Public License v. 2.0 for more details.

For more details, see http://www.mrtrix.org/.


