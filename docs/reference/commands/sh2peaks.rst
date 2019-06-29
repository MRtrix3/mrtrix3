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

Description
-----------

The spherical harmonic coefficients are stored as follows. First, since the signal attenuation profile is real, it has conjugate symmetry, i.e. Y(l,-m) = Y(l,m)* (where * denotes the complex conjugate). Second, the diffusion profile should be antipodally symmetric (i.e. S(x) = S(-x)), implying that all odd l components should be zero. Therefore, only the even elements are computed. Note that the spherical harmonics equations used here differ slightly from those conventionally used, in that the (-1)^m factor has been omitted. This should be taken into account in all subsequent calculations. Each volume in the output image corresponds to a different spherical harmonic component. Each volume will correspond to the following: volume 0: l = 0, m = 0 ; volume 1: l = 2, m = 2 (imaginary part) ; volume 2: l = 2, m = 1 (imaginary part) ; volume 3: l = 2, m = 0 ; volume 4: l = 2, m = 1 (real part) ; volume 5: l = 2, m = 2 (real part) ; volume 6: l = 4, m = 4 (imaginary part) ; volume 7: l = 4, m = 3 (imaginary part) ; etc...

Options
-------

-  **-num peaks** the number of peaks to extract (default: 3).

-  **-direction phi theta**  *(multiple uses permitted)* the direction of a peak to estimate. The algorithm will attempt to find the same number of peaks as have been specified using this option.

-  **-peaks image** the program will try to find the peaks that most closely match those in the image provided.

-  **-threshold value** only peak amplitudes greater than the threshold will be considered.

-  **-seeds file** specify a set of directions from which to start the multiple restarts of the optimisation (by default, the built-in 60 direction set is used)

-  **-mask image** only perform computation within the specified binary brain mask image.

-  **-fast** use lookup table to compute associated Legendre polynomials (faster, but approximate).

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


