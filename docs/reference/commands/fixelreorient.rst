.. _fixelreorient:

fixelreorient
===================

Synopsis
--------

Reorient fixel directions

Usage
--------

::

    fixelreorient [ options ]  fixel_in warp fixel_out

-  *fixel_in*: the fixel directory
-  *warp*: a 4D deformation field used to perform reorientation. Reorientation is performed by applying the Jacobian affine transform in each voxel in the warp, then re-normalising the vector representing the fixel direction
-  *fixel_out*: the output fixel directory. If the the input and output directorys are the same, the existing directions file will be replaced (providing the --force option is supplied). If a new directory is supplied then the fixel directions and all other fixel data will be copied to the new directory.

Description
-----------

Reorientation is performed by transforming the vector representing the fixel direction with the Jacobian (local affine transform) computed at each voxel in the warp, then re-normalising the vector.

Options
-------

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

-  **-failonwarn** terminate program if a warning is produced

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------



**Author:** David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


