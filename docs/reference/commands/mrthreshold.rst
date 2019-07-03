.. _mrthreshold:

mrthreshold
===================

Synopsis
--------

Create bitwise image by thresholding image intensity

Usage
--------

::

    mrthreshold [ options ]  input[ output ]

-  *input*: the input image to be thresholded
-  *output*: the (optional) output binary image mask

Description
-----------

The threshold value to be applied can be determined in one of a number of ways:

- If no relevant command-line option is used, the command will automatically determine an optimal threshold;

- The -abs option provides the threshold value explicitly;

- The -percentile, -top and -bottom options enable more fine-grained control over how the threshold value is determined.

The -mask option only influences those image values that contribute toward the determination of the threshold value; once the threshold is determined, it is applied to the entire image, irrespective of use of the -mask option.

If no output image path is specified, the command will instead write to standard output the determined threshold value.

Options
-------

Different mechanisms for determining the threshold value (use no more than one)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-abs value** specify threshold value as absolute intensity

-  **-percentile value** determine threshold based on some percentile of the image intensity distribution

-  **-top count** determine threshold that will result in selection of some number of top-valued voxels

-  **-bottom count** determine threshold that will result in omission of some number of bottom-valued voxels

Options that influence determination of the threshold based on the input image
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-ignorezero** ignore zero-valued input values

-  **-mask image** compute the threshold based only on values within an input mask image

Options that influence generation of the output image after the threshold is determined
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-invert** invert the output binary mask

-  **-nan** set voxels that fail the threshold to NaN rather than zero.

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status; alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files (caution: using the same file as input and output might cause unexpected behaviour).

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-config key value**  *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

* If not using any explicit thresholding mechanism: Ridgway, G. R.; Omar, R.; Ourselin, S.; Hill, D. L.; Warren, J. D. & Fox, N. C. Issues with threshold masking in voxel-based morphometry of atrophied brains. NeuroImage, 2009, 44, 99-111

--------------



**Author:** J-Donald Tournier (jdtournier@gmail.com) and Robert E. Smith (robert.smith@florey.edu.au)

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


