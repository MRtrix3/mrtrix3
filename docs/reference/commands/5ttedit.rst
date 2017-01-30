.. _5ttedit:

5ttedit
===========

Synopsis
--------

::

    5ttedit [ options ]  input output

-  *input*: the 5TT image to be modified
-  *output*: the output modified 5TT image

Description
-----------

manually set the partial volume fractions in an ACT five-tissue-type (5TT) image using mask images

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

-  **-quiet** do not display information messages or progress status.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

-  **-failonwarn** terminate program if a warning is produced

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.

