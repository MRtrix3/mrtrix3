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

-  *fixel_in*: the input fixel directory
-  *warp*: a 4D deformation field used to perform reorientation.
-  *fixel_out*: the output fixel directory. If the the input and output directories are the same, the existing directions file will be replaced (providing the -force option is supplied). If a new directory is supplied, then the fixel directions and all other fixel data will be copied to the new directory.

Description
-----------

Whenever data that encode some orientation-dependent data are transformed in space, there is a corresponding rotation of that orientation-dependent data that must occur. Typically, spatial transformation and reorientation of data should happen simultaneously. This command however operates in a very specific context where this is NOT the case. If the data from which fixels are estimated have been transformed in space, but the corresponding requisite reorientation that should accompany such a transformation was NOT applied, then that reorientation can instead be applied to the fixel directions after the fact. The most common scenario is where FODs are transformed from one space to another, but FOD-based reorientation is explicitly disabled during such due to its potentially deleterious consequences on FOD shape, with the requisite reorientation instead applied to the fixels resulting from FOD segmentation.

Reorientation is performed by transforming the vector representing the fixel direction with the Jacobian (local affine transform) computed at each voxel in the warp, then re-normalising the vector.

Fixel data are stored utilising the fixel directory format described in the main documentation, which can be found at the following link:  |br|
https://mrtrix.readthedocs.io/en/3.0.8/fixel_based_analysis/fixel_directory_format.html

Options
-------

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



**Author:** David Raffelt (david.raffelt@florey.edu.au)

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


