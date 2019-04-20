.. _fixel2tsf:

fixel2tsf
===================

Synopsis
--------

Map fixel values to a track scalar file based on an input tractogram

Usage
--------

::

    fixel2tsf [ options ]  fixel_in tracks tsf

-  *fixel_in*: the input fixel data file (within the fixel directory)
-  *tracks*: the input track file 
-  *tsf*: the output track scalar file

Description
-----------

This command is useful for visualising all brain fixels (e.g. the output from fixelcfestats) in 3D.

Options
-------

-  **-angle value** the max anglular threshold for computing correspondence between a fixel direction and track tangent (default = 45 degrees)

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status; alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files (caution: using the same file as input and output might cause unexpected behaviour).

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-config key value**  *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------



**Author:** David Raffelt (david.raffelt@florey.edu.au)

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


