.. _shbasis:

shbasis
===================

Synopsis
--------

Examine the values in spherical harmonic images to estimate (and optionally change) the SH basis used

Usage
--------

::

    shbasis [ options ]  SH [ SH ... ]

-  *SH*: the input image(s) of SH coefficients.

Description
-----------

In previous versions of MRtrix, the convention used for storing spherical harmonic coefficients was a non-orthonormal basis (the m!=0 coefficients were a factor of sqrt(2) too large). This error has been rectified in the new MRtrix (assuming that compilation was performed without the USE_NON_ORTHONORMAL_SH_BASIS symbol defined), but will cause issues if processing SH data that was generated using an older version of MRtrix (or vice-versa).

This command provides a mechanism for testing the basis used in storage of image data representing a spherical harmonic series per voxel, and allows the user to forcibly modify the raw image data to conform to the desired basis.

Options
-------

-  **-convert mode** convert the image data in-place to the desired basis (if necessary). Options are: old, new, native (whichever basis MRtrix is compiled for; most likely the new orthonormal basis), force_oldtonew, force_newtoold. Note that for the "force_*" choices should ideally only be used in cases where the command is unable to automatically determine the SH basis using the existing image data.

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



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


