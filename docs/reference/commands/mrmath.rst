.. _mrmath:

mrmath
===========

Synopsis
--------

::

    mrmath [ options ]  input [ input ... ] operation output

-  *input*: the input image(s).
-  *operation*: the operation to apply, one of: mean, median, sum, product, rms, norm, var, std, min, max, absmax, magmax.
-  *output*: the output image.

Description
-----------

compute summary statistic on image intensities either across images, or along a specified axis for a single image. Supported operations are:

mean, median, sum, product, rms (root-mean-square value), norm (vector 2-norm), var (unbiased variance), std (unbiased standard deviation), min, max, absmax (maximum absolute value), magmax (value with maximum absolute value, preserving its sign).

See also 'mrcalc' to compute per-voxel operations.

Options
-------

-  **-axis index** perform operation along a specified axis of a single input image

Data type options
^^^^^^^^^^^^^^^^^

-  **-datatype spec** specify output image data type. Valid choices are: float32, float32le, float32be, float64, float64le, float64be, int64, uint64, int64le, uint64le, int64be, uint64be, int32, uint32, int32le, uint32le, int32be, uint32be, int16, uint16, int16le, uint16le, int16be, uint16be, cfloat32, cfloat32le, cfloat32be, cfloat64, cfloat64le, cfloat64be, int8, uint8, bit.

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

