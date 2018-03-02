.. _mrhistogram:

mrhistogram
===================

Synopsis
--------

Generate a histogram of image intensities

Usage
--------

::

    mrhistogram [ options ]  image hist

-  *image*: the input image from which the histogram will be computed
-  *hist*: the output histogram file

Options
-------

Histogram generation options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-bins num** Manually set the number of bins to use to generate the histogram.

-  **-template file** Use an existing histogram file as the template for histogram formation

-  **-mask image** Calculate the histogram only within a mask image.

-  **-ignorezero** ignore zero-valued data during histogram construction.

Additional options for mrhistogram
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-allvolumes** generate one histogram across all image volumes, rather than one per image volume

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



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2018 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/

MRtrix3 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/


