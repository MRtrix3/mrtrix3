.. _fixel2sh:

fixel2sh
===================

Synopsis
--------

Convert a fixel-based sparse-data image into an spherical harmonic image

Usage
--------

::

    fixel2sh [ options ]  fixel_in sh_out

-  *fixel_in*: the input fixel data file.
-  *sh_out*: the output sh image.

Description
-----------

This command generates spherical harmonic data from fixels that can be visualised using the ODF tool in MRview. The output ODF lobes are scaled according to the values in the input fixel image.

Options
-------

-  **-lmax order** set the maximum harmonic order for the output series (Default: 8)

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



**Author:** Robert E. Smith (robert.smith@florey.edu.au) & David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


