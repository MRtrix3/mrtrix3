.. _peaksvalidate:

peaksvalidate
===================

Synopsis
--------

Validate a so-called peaks image (a set of 3-vectors per voxel where each encodes an orientation)

Usage
--------

::

    peaksvalidate [ options ]  image

-  *image*: the input peaks image

Description
-----------

This command checks that an image conforms to the requirements of a peaks image, in which successive triplets of volumes encode the (x, y, z) Cartesian components of one peak direction per triplet.

The following checks are performed:

1. The image must be of floating-point type.

2. The image must be 4-dimensional, with the number of volumes a multiple of three.

3. Where a voxel contains fewer peaks than the maximum, the unfilled peak slots must use a single consistent fill convention across the entire image: either all three components of the fill triplet are zero, or all three components are NaN. Both conventions must not be mixed within the same image.

4. When the fill convention is NaN, every individual triplet must be either entirely finite or entirely NaN. Triplets that are partly NaN and partly non-NaN are not valid.

5. Every non-fill peak must contain finite values.

The command also reports the range of norms across all non-fill peaks, indicating whether the image stores unit-norm directions only or whether a quantitative value is associated with each peak (encoded in the vector norm).

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


