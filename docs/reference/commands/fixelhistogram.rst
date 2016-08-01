.. _fixelhistogram:

fixelhistogram
===========

Synopsis
--------

::

    fixelhistogram [ options ]  input

-  *input*: the input fixel image.

Description
-----------

Generate a histogram of fixel values.

Options
-------

Histogram generation options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-bins num** Manually set the number of bins to use to generate the histogram.

-  **-template file** Use an existing histogram file as the template for histogram formation

-  **-mask image** Calculate the histogram only within a mask image.

-  **-ignorezero** ignore zero-valued data during histogram construction.

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

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

