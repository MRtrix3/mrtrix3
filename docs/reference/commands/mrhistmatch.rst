.. _mrhistmatch:

mrhistmatch
===================

Synopsis
--------

Modify the intensities of one image to match the histogram of another via a non-linear transform

Usage
--------

::

    mrhistmatch [ options ]  input target output

-  *input*: the input image to be modified
-  *target*: the input image from which to derive the target histogram
-  *output*: the output image

Options
-------

-  **-mask_input image** only generate input histogram based on a specified binary mask image

-  **-mask_target image** only generate target histogram based on a specified binary mask image

-  **-cdfs path** output the histogram CDFs to a text file (for debugging).

-  **-bins num** the number of bins to use to generate the histograms

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

* If using inverse contrast normalization for inter-modal (DWI - T1) registration:Bhushan, C.; Haldar, J. P.; Choi, S.; Joshi, A. A.; Shattuck, D. W. & Leahy, R. M. Co-registration and distortion correction of diffusion and anatomical images based on inverse contrast normalization. NeuroImage, 2015, 115, 269-280

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au

**Copyright:** Copyright (c) 2008-2018 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/

MRtrix3 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/


