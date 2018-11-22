.. _sh2power:

sh2power
===================

Synopsis
--------

Compute the total power of a spherical harmonics image

Usage
--------

::

    sh2power [ options ]  SH power

-  *SH*: the input spherical harmonics coefficients image.
-  *power*: the output power image.

Description
-----------

This command computes the sum of squared SH coefficients, which equals the mean-squared amplitude of the spherical function it represents.

Options
-------

-  **-spectrum** output the power spectrum, i.e., the power contained within each harmonic degree (l=0, 2, 4, ...) as a 4-D image.

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
See the Mozila Public License v. 2.0 for more details.

For more details, see http://www.mrtrix.org/.


