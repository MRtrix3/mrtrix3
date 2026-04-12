.. _labelvalidate:

labelvalidate
===================

Synopsis
--------

Validate a hard segmentation (label) image

Usage
--------

::

    labelvalidate [ options ]  image

-  *image*: the input label image

Description
-----------

This command checks that an image conforms to the requirements of a hard segmentation image. Specifically, it verifies that the image is 3-dimensional (or 4-dimensional with a singleton 4th axis), and contains only non-negative integer values (either stored as an unsigned integer datatype, or as a floating-point or signed integer datatype with values that satisfy these constraints).

The command then reports two further properties of the image. First, whether the label indices are contiguous: i.e. whether all integer values in the range [1, max_label] are present in the image. Second, for each unique non-background label, whether the set of voxels carrying that label forms a single spatially connected region or comprises multiple disconnected components, assessed using 26-nearest-neighbour voxel connectivity.

Label index 0 is treated as background and excluded from all reports.

Options
-------

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


