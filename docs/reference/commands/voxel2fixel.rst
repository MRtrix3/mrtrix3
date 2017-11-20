.. _voxel2fixel:

voxel2fixel
===================

Synopsis
--------

Map the scalar value in each voxel to all fixels within that voxel

Usage
--------

::

    voxel2fixel [ options ]  image_in fixel_directory_in fixel_directory_out fixel_data_out

-  *image_in*: the input image.
-  *fixel_directory_in*: the input fixel directory. Used to define the fixels and their directions
-  *fixel_directory_out*: the output fixel directory. This can be the same as the input directory if desired
-  *fixel_data_out*: the name of the fixel data image.

Description
-----------

This command is designed to enable CFE-based statistical analysis to be performed on voxel-wise measures.

Options
-------

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

-  **-failonwarn** terminate program if a warning is produced

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------



**Author:** David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


