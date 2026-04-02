.. _5ttedit:

5ttedit
===================

Synopsis
--------

Manually set the partial volume fractions in an ACT five-tissue-type (5TT) image

Usage
--------

::

    5ttedit [ options ]  input output

-  *input*: the 5TT image to be modified
-  *output*: the output modified 5TT image

Description
-----------

The user-provided images are interpreted as desired partial volume fractions in the output image. For any voxel with a non-zero value in such an image, this will be the value for that tissue in the output 5TT image. For tissues for which the user has not explicitly requested a change to the partial volume fraction, the partial volume fraction may nevertheless change in order to preserve the requirement of a unity sum of partial volume fractions in each voxel.

Any voxels that are included in the mask provided by the -none option will be erased in the output image; this supersedes all other user inputs.

Options
-------

-  **-cgm image** provide an image of new cortical grey matter partial volume fractions

-  **-sgm image** provide an image of new sub-cortical grey matter partial volume fractions

-  **-wm image** provide an image of new white matter partial volume fractions

-  **-csf image** provide an image of new CSF partial volume fractions

-  **-path image** provide an image of new pathological tissue partial volume fractions

-  **-none image** provide a mask of voxels that should be cleared (i.e. are non-brain)

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


