.. _mrcheckerboardmask:

mrcheckerboardmask
===================

Synopsis
--------

Create bitwise checkerboard image

Usage
--------

::

    mrcheckerboardmask [ options ]  input output

-  *input*: the input image to be used as a template.
-  *output*: the output binary image mask.

Options
-------

-  **-tiles value** specify the number of tiles in any direction

-  **-invert** invert output binary mask.

-  **-nan** use NaN as the output zero value.

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



**Author:** Max Pietsch (maximilian.pietsch@kcl.ac.uk)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


