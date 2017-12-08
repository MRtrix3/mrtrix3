.. _mrthreshold:

mrthreshold
===================

Synopsis
--------

Create bitwise image by thresholding image intensity

Usage
--------

::

    mrthreshold [ options ]  input output

-  *input*: the input image to be thresholded.
-  *output*: the output binary image mask.

Description
-----------

By default, an optimal threshold is determined using a parameter-free method. Alternatively the threshold can be defined manually by the user.

Options
-------

-  **-abs value** specify threshold value as absolute intensity.

-  **-percentile value** threshold the image at the ith percentile.

-  **-top N** provide a mask of the N top-valued voxels

-  **-bottom N** provide a mask of the N bottom-valued voxels

-  **-invert** invert output binary mask.

-  **-toppercent N** provide a mask of the N%% top-valued voxels

-  **-bottompercent N** provide a mask of the N%% bottom-valued voxels

-  **-nan** use NaN as the output zero value.

-  **-ignorezero** ignore zero-valued input voxels.

-  **-mask image** compute the optimal threshold based on voxels within a mask.

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

* If not using any manual thresholding option:Ridgway, G. R.; Omar, R.; Ourselin, S.; Hill, D. L.; Warren, J. D. & Fox, N. C. Issues with threshold masking in voxel-based morphometry of atrophied brains. NeuroImage, 2009, 44, 99-111

--------------



**Author:** J-Donald Tournier (jdtournier@gmail.com)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


