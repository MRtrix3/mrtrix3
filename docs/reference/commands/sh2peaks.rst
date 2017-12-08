.. _sh2peaks:

sh2peaks
===================

Synopsis
--------

Extract the peaks of a spherical harmonic function at each voxel, by commencing a Newton search along a set of specified directions

Usage
--------

::

    sh2peaks [ options ]  SH output

-  *SH*: the input image of SH coefficients.
-  *output*: the output image. Each volume corresponds to the x, y & z component of each peak direction vector in turn.

Options
-------

-  **-num peaks** the number of peaks to extract (default: 3).

-  **-direction phi theta** the direction of a peak to estimate. The algorithm will attempt to find the same number of peaks as have been specified using this option.

-  **-peaks image** the program will try to find the peaks that most closely match those in the image provided.

-  **-threshold value** only peak amplitudes greater than the threshold will be considered.

-  **-seeds file** specify a set of directions from which to start the multiple restarts of the optimisation (by default, the built-in 60 direction set is used)

-  **-mask image** only perform computation within the specified binary brain mask image.

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

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


