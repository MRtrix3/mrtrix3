.. _fixelcrop:

fixelcrop
===========

Synopsis
--------

::

    fixelcrop [ options ]  input_fixel_folder input_fixel_data_mask output_fixel_folder

-  *input_fixel_folder*: the input fixel folder file to be cropped
-  *input_fixel_data_mask*: the input fixel data file to be cropped
-  *output_fixel_folder*: the output fixel folder

Description
-----------

Crop a fixel index image (i.e. remove fixels) using a fixel mask

Options
-------

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



**Author:** David Raffelt (david.raffelt@florey.edu.au) & Rami Tabarra (rami.tabarra@florey.edu.au)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

