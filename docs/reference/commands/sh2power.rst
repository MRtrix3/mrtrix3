.. _sh2power:

sh2power
===========

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

-  **-quiet** do not display information messages or progress status.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

-  **-failonwarn** terminate program if a warning is produced

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------



**Author:** J-Donald Tournier (jdtournier@gmail.com)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.

