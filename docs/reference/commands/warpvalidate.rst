.. _warpvalidate:

warpvalidate
===================

Synopsis
--------

Validate a non-linear warp image

Usage
--------

::

    warpvalidate [ options ]  image

-  *image*: the input warp image

Description
-----------

This command checks that an image conforms to the requirements of a non-linear warp field.

The format is inferred from the image dimensions:

4D image with 3 volumes: a displacement or deformation field. The image must be 4-dimensional with exactly 3 volumes in the 4th dimension (the x, y, z warp components). MRtrix3 uses displacement and deformation fields interchangeably; the structural requirements are identical for both.

5D image with 3 volumes for each of 4 volume groups: a full warp field as produced by eg. mrregister -nl_warp_full. The image must be 5-dimensional, with exactly 3 volumes in the 4th dimension (x/y/z components of the warp) and exactly 4 volume groups in the 5th dimension (image1-to-midway, midway-to-image1, image2-to-midway, midway-to-image2). The header must also contain "linear1" and "linear2" fields encoding the linear transform component for each image.

The following checks are performed:

1. The image must be of floating-point type.

2. The dimensionality and volume counts must match one of the two supported formats described above.

3. For full warp images, the header must contain both the "linear1" and "linear2" linear transform fields.

4. Fill values, used to mark voxels that lie outside the domain of the warp (e.g. outside a brain mask), must use a single consistent convention across the entire image: either all fill triplets are all-zero, or all fill triplets are all-NaN. Both conventions must not be mixed within the same image.

5. Where the fill convention is NaN, every individual warp triplet must be either entirely finite or entirely NaN. Triplets that are partly NaN and partly non-NaN are not valid.

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


