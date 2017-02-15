.. _fixelconvert:

fixelconvert
===========

Synopsis
--------

Convert between the old format fixel image (*.msf / *.msh) and the new fixel directory format

Usage
--------

::

    fixelconvert [ options ]  fixel_in fixel_out

-  *fixel_in*: the input fixel file / directory.
-  *fixel_out*: the output fixel file / directory.
Options
-------

Options for converting from old to new format
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-name string** assign a different name to the value field output (Default: value). Do not include the file extension.

-  **-nii** output the index, directions and data file in NIfTI format instead of *.mif

-  **-out_size** also output the 'size' field from the old format

-  **-template path** specify an existing fixel directory (in the new format) to which the new output should conform

Options for converting from new to old format
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-value path** nominate the data file to import to the 'value' field in the old format

-  **-in_size path** import data for the 'size' field in the old format

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



**Author:** David Raffelt (david.raffelt@florey.edu.au) and Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.

