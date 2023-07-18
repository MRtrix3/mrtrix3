.. _mrmath:

mrmath
===================

Synopsis
--------

Compute summary statistic on image intensities either across images, or along a specified axis of a single image

Usage
--------

::

    mrmath [ options ]  input [ input ... ] operation output

-  *input*: the input image(s).
-  *operation*: the operation to apply, one of: mean, median, sum, product, rms, norm, var, std, min, max, absmax, magmax.
-  *output*: the output image.

Description
-----------

Supported operations are:

mean, median, sum, product, rms (root-mean-square value), norm (vector 2-norm), var (unbiased variance), std (unbiased standard deviation), min, max, absmax (maximum absolute value), magmax (value with maximum absolute value, preserving its sign).

This command is used to traverse either along an image axis, or across a set of input images, calculating some statistic from the values along each traversal. If you are seeking to instead perform mathematical calculations that are done independently for each voxel, pleaase see the 'mrcalc' command.

Example usages
--------------

-   *Calculate a 3D volume representing the mean intensity across a 4D image series*::

        $ mrmath 4D.mif mean 3D_mean.mif -axis 3

    This is a common operation for calculating e.g. the mean value within a specific DWI b-value. Note that axis indices start from 0; thus, axes 0, 1 & 2 are the three spatial axes, and axis 3 operates across volumes.

-   *Generate a Maximum Intensity Projection (MIP) along the inferior-superior direction*::

        $ mrmath input.mif max MIP.mif -axis 2

    Since a MIP is literally the maximal value along a specific projection direction, axis-aligned MIPs can be generated easily using mrmath with the 'max' operation.

Options
-------

-  **-axis index** perform operation along a specified axis of a single input image

-  **-keep_unary_axes** Keep unary axes in input images prior to calculating the stats. The default is to wipe axes with single elements.

Data type options
^^^^^^^^^^^^^^^^^

-  **-datatype spec** specify output image data type. Valid choices are: float32, float32le, float32be, float64, float64le, float64be, int64, uint64, int64le, uint64le, int64be, uint64be, int32, uint32, int32le, uint32le, int32be, uint32be, int16, uint16, int16le, uint16le, int16be, uint16be, cfloat32, cfloat32le, cfloat32be, cfloat64, cfloat64le, cfloat64be, int8, uint8, bit.

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



**Author:** J-Donald Tournier (jdtournier@gmail.com)

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


