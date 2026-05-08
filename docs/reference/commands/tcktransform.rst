.. _tcktransform:

tcktransform
===================

Synopsis
--------

Apply a spatial transformation to a tracks file

Usage
--------

::

    tcktransform [ options ]  tracks transform output

-  *tracks*: the input track file.
-  *transform*: the image containing the transform.
-  *output*: the output track file

Description
-----------

Unlike the non-linear transformation of image data, where the value of the deformation field in a destination voxel position defines the location in space from which to "pull" image data into that voxel, the non-linear transformation of streamlines data involves sampling the deformation field at each streamline vertex location to determine the new spatial location to which to "push" that vertex. As such, the appropriate deformation field to apply to streamlines data is the inverse of what would be applied to image data. So for instance, this may involve the utilisation of a template-to-subject warp field in order to transform streamlines from subject to template space.

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



**Author:** J-Donald Tournier (jdtournier@gmail.com)

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


