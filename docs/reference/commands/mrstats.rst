.. _mrstats:

mrstats
===================

Synopsis
--------

Compute images statistics

Usage
--------

::

    mrstats [ options ]  image

-  *image*: the input image from which statistics will be computed.

Options
-------

Statistics options
^^^^^^^^^^^^^^^^^^

-  **-output field**  *(multiple uses permitted)* output only the field specified. Multiple such options can be supplied if required. Choices are: mean, median, std, std_rv, min, max, count. Useful for use in scripts. Both std options refer to the unbiased (sample) standard deviation. For complex data, min, max and std are calculated separately for real and imaginary parts, std_rv is based on the real valued variance (equals sqrt of sum of variances of imaginary and real parts).

-  **-mask image** only perform computation within the specified binary mask image.

-  **-ignorezero** ignore zero values during statistics calculation

Additional options for mrstats
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-allvolumes** generate statistics across all image volumes, rather than one set of statistics per image volume

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

--------------



**Author:** J-Donald Tournier (jdtournier@gmail.com)

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


