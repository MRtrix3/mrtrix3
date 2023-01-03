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

In previous versions of MRtrix, the convention used for storing spherical harmonic coefficients was a non-orthonormal basis (the m!=0 coefficients were a factor of sqrt(2) too large). This error has been rectified in newer versions of MRtrix, but will cause issues if processing SH data that was generated using an older version of MRtrix (or vice-versa).

This command provides a mechanism for testing the basis used in storage of image data representing a spherical harmonic series per voxel, and allows the user to forcibly modify the raw image data to conform to the desired basis.

Note that the "force_*" conversion choices should only be used in cases where this command has previously been unable to automatically determine the SH basis from the image data, but the user themselves are confident of the SH basis of the data.

The spherical harmonic coefficients are stored according the conventions described the main documentation, which can be found at the following link:  |br|
https://mrtrix.readthedocs.io/en/3.0.4/concepts/spherical_harmonics.html

Options
-------

-  **-convert mode** convert the image data in-place to the desired basis; options are: old,new,force_oldtonew,force_newtoold.

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



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

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


