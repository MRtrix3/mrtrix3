.. _mraverageheader:

mraverageheader
===================

Synopsis
--------

Calculate the average (unbiased) coordinate space of all input images

Usage
--------

::

    mraverageheader [ options ]  input [ input ... ] output

-  *input*: the input image(s).
-  *output*: the output image

Description
-----------

The voxel spacings of the calculated average header can be determined from the set of input images in one of four different ways, which may be more or less appropriate in different use cases. These options vary in two key ways: 1. Projected voxel spacing of the input image in the direction of the average header axes versus the voxel spacing of the input image axis that is closest to the average header axis; 2. Selecting the minimum of these spacings across input images to maintain maximal spatial resolution versus the mean across images to produce an unbiased average.

Options
-------

-  **-padding value**  boundary box padding in voxels. Default: 0

-  **-spacing type** Method for determination of voxel spacings based on the set of input images and the average header axes (see Description).Valid options are: min_projection,mean_projection,min_nearest,mean_nearest; default = mean_nearest

-  **-fill** set the intensity in the first volume of the average space to 1

Data type options
^^^^^^^^^^^^^^^^^

-  **-datatype spec** specify output image data type. Valid choices are: float16, float16le, float16be, float32, float32le, float32be, float64, float64le, float64be, int64, uint64, int64le, uint64le, int64be, uint64be, int32, uint32, int32le, uint32le, int32be, uint32be, int16, uint16, int16le, uint16le, int16be, uint16be, cfloat16, cfloat16le, cfloat16be, cfloat32, cfloat32le, cfloat32be, cfloat64, cfloat64le, cfloat64be, int8, uint8, bit.

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



**Author:** Maximilian Pietsch (maximilian.pietsch@kcl.ac.uk)

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


