.. _sh2peaks:

sh2peaks
===================

Synopsis
--------

Extract the peaks of a spherical harmonic function in each voxel

Usage
--------

::

    sh2peaks [ options ]  SH output

-  *SH*: the input image of SH coefficients.
-  *output*: the output image. Each volume corresponds to the x, y & z component of each peak direction vector in turn.

Description
-----------

Peaks of the spherical harmonic function in each voxel are located by commencing a Newton search along each of a set of pre-specified directions

The spherical harmonic coefficients are stored according the conventions described the main documentation, which can be found at the following link:  |br|
https://mrtrix.readthedocs.io/en/3.0.4/concepts/spherical_harmonics.html

Options
-------

-  **-num peaks** the number of peaks to extract (default: 3).

-  **-direction phi theta** *(multiple uses permitted)* the direction of a peak to estimate. The algorithm will attempt to find the same number of peaks as have been specified using this option.

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

-  **-config key value** *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

Jeurissen, B.; Leemans, A.; Tournier, J.-D.; Jones, D.K.; Sijbers, J. Investigating the prevalence of complex fiber configurations in white matter tissue with diffusion magnetic resonance imaging. Human Brain Mapping, 2013, 34(11), 2747-2766

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** J-Donald Tournier (jdtournier@gmail.com)

**Copyright:** Copyright (c) 2008-2023 the MRtrix3 contributors.

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


