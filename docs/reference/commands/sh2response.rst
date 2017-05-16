.. _sh2response:

sh2response
===================

Synopsis
--------

Generate an appropriate response function from the image data for spherical deconvolution

Usage
--------

::

    sh2response [ options ]  SH mask directions response

-  *SH*: the spherical harmonic decomposition of the diffusion-weighted images
-  *mask*: the mask containing the voxels from which to estimate the response function
-  *directions*: a 4D image containing the direction vectors along which to estimate the response function
-  *response*: the output axially-symmetric spherical harmonic coefficients

Options
-------

-  **-lmax value** specify the maximum harmonic degree of the response function to estimate

-  **-dump file** dump the m=0 SH coefficients from all voxels in the mask to the output file, rather than their mean

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

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


