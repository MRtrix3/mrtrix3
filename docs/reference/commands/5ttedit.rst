.. _5ttedit:

5ttedit
===================

Synopsis
--------

Manually set the partial volume fractions in an ACT five-tissue-type (5TT) image using mask images

Usage
--------

::

    5ttedit [ options ]  input output

-  *input*: the 5TT image to be modified
-  *output*: the output modified 5TT image

Options
-------

-  **-cgm image** provide a mask of voxels that should be set to cortical grey matter

-  **-sgm image** provide a mask of voxels that should be set to sub-cortical grey matter

-  **-wm image** provide a mask of voxels that should be set to white matter

-  **-csf image** provide a mask of voxels that should be set to CSF

-  **-path image** provide a mask of voxels that should be set to pathological tissue

-  **-none image** provide a mask of voxels that should be cleared (i.e. are non-brain); note that this will supersede all other provided masks

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2019 the MRtrix3 contributors.

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


