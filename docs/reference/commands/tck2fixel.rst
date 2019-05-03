.. _tck2fixel:

tck2fixel
===================

Synopsis
--------

Compute a fixel TDI map from a tractogram

Usage
--------

::

    tck2fixel [ options ]  tracks fixel_folder_in fixel_folder_out fixel_data_out

-  *tracks*: the input tracks.
-  *fixel_folder_in*: the input fixel folder. Used to define the fixels and their directions
-  *fixel_folder_out*: the fixel folder to which the output will be written. This can be the same as the input folder if desired
-  *fixel_data_out*: the name of the fixel data image.

Options
-------

-  **-angle value** the max angle threshold for assigning streamline tangents to fixels (Default: 45 degrees)

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


