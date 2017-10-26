.. _fixelcrop:

fixelcrop
===================

Synopsis
--------

Crop/remove fixels from sparse fixel image using a binary fixel mask

Usage
--------

::

    fixelcrop [ options ]  input_fixel_directory input_fixel_mask output_fixel_directory

-  *input_fixel_directory*: input fixel directory, all data files and directions file will be cropped and saved in the output fixel directory
-  *input_fixel_mask*: the input fixel data file defining which fixels to crop. Fixels with zero values will be removed
-  *output_fixel_directory*: the output directory to store the cropped directions and data files

Description
-----------

The mask must be input as a fixel data file the same dimensions as the fixel data file(s) to be cropped.

Options
-------

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------



**Author:** David Raffelt (david.raffelt@florey.edu.au) & Rami Tabarra (rami.tabarra@florey.edu.au)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


