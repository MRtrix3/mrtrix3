.. _fixel2tsf:

fixel2tsf
===========

Synopsis
--------

::

    fixel2tsf [ options ]  fixel_in tracks tsf

-  *fixel_in*: the input fixel data file (within the fixel directory)
-  *tracks*: the input track file 
-  *tsf*: the output track scalar file

Description
-----------

Map fixel values to a track scalar file based on an input tractogram. This is useful for visualising all brain fixels (e.g. the output from fixelcfestats) in 3D.

Options
-------

-  **-angle value** the max anglular threshold for computing correspondence between a fixel direction and track tangent (default = 45 degrees)

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



**Author:** David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

