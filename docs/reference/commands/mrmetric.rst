.. _mrmetric:

mrmetric
===================

Synopsis
--------

Computes a dissimilarity metric between two images

Usage
--------

::

    mrmetric [ options ]  image1 image2

-  *image1*: the first input image.
-  *image2*: the second input image.

Description
-----------

Currently only the mean squared difference is fully implemented.

Options
-------

-  **-space iteration method** voxel (default): per voxel image1: scanner space of image 1 image2: scanner space of image 2 average: scanner space of the average affine transformation of image 1 and 2 

-  **-interp method** set the interpolation method to use when reslicing (choices: nearest, linear, cubic, sinc. Default: linear).

-  **-metric method** define the dissimilarity metric used to calculate the cost. Choices: diff (squared differences), cc (non-normalised negative cross correlation aka negative cross covariance). Default: diff). cc is only implemented for -space average and -interp linear and cubic.

-  **-mask1 image** mask for image 1

-  **-mask2 image** mask for image 2

-  **-nonormalisation** do not normalise the dissimilarity metric to the number of voxels.

-  **-overlap** output number of voxels that were used.

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



**Author:** David Raffelt (david.raffelt@florey.edu.au) and Max Pietsch (maximilian.pietsch@kcl.ac.uk)

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


