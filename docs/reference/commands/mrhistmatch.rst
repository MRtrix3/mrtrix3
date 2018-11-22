.. _mrhistmatch:

mrhistmatch
===================

Synopsis
--------

Modify the intensities of one image to match the histogram of another

Usage
--------

::

    mrhistmatch [ options ]  type input target output

-  *type*: type of histogram matching to perform; options are: scale,linear,nonlinear
-  *input*: the input image to be modified
-  *target*: the input image from which to derive the target histogram
-  *output*: the output image

Options
-------

Image masking options
^^^^^^^^^^^^^^^^^^^^^

-  **-mask_input image** only generate input histogram based on a specified binary mask image

-  **-mask_target image** only generate target histogram based on a specified binary mask image

Non-linear histogram matching options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2019 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Covered Software is provided under this License on an "as is"
basis, without warranty of any kind, either expressed, implied, or
statutory, including, without limitation, warranties that the
Covered Software is free of defects, merchantable, fit for a
particular purpose or non-infringing.
See the Mozila Public License v. 2.0 for more details.

For more details, see http://www.mrtrix.org/.


