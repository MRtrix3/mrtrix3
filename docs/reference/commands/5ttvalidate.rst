.. _5ttvalidate:

5ttvalidate
===================

Synopsis
--------

Validate that one or more images conform to the expected ACT five-tissue-type (5TT) format

Usage
--------

::

    5ttvalidate [ options ]  input [ input ... ]

-  *input*: the 5TT image(s) to be validated

Description
-----------

A 5TT image encodes the partial volume fractions of five tissue types in every voxel: cortical grey matter, sub-cortical grey matter, white matter, CSF, and pathological tissue. Each tissue partial volume fraction (PVF) must be a value in [0.0, 1.0], and for any brain voxel the five PVFs must sum to 1.0.

The following checks are performed on each input image:

1. The image must be of floating-point type, 4-dimensional, and contain exactly 5 volumes. Failure causes the image to be immediately rejected as structurally invalid.

2. For every brain voxel (identified by a non-zero partial-volume sum), each of the five tissue PVFs must lie within [0.0, 1.0]. Voxels that violate this constraint contain non-physical values and the image is rejected as a hard error.

3. For every brain voxel, the sum of all five tissue PVFs must equal 1.0 to within a tolerance of 

0.001

. Voxels that violate this constraint are reported as a soft warning: the image may still be usable for ACT but does not perfectly conform to the format.

Options
-------

-  **-voxels prefix** output mask images highlighting voxels where the input does not conform to 5TT requirements

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status; alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages & debug input data.

-  **-force** force overwrite of output files (caution: using the same file as input and output might cause unexpected behaviour).

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-config key value** *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2026 the MRtrix3 contributors.

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


