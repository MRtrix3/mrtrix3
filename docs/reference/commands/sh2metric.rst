.. _sh2metric:

sh2metric
===================

Synopsis
--------

Compute voxel-wise metrics from one or more spherical harmonics images

Usage
--------

::

    sh2metric [ options ]  SH [ SH ... ] metric power

-  *SH*: the input spherical harmonics coefficients image
-  *metric*: the metrc to compute; one of: entropy,power
-  *power*: the output metric image

Description
-----------

Depending on the particular metric being computed, the command may only accept a single input SH image; wheras other metrics may accept multiple SH images as input (eg. ODFs) and compute a single scalar output image.

The various metrics available are detailed individually below.

"entropy": this metric computes the entropy (in nits, ie. logarithm base e) of one or more spherical harmonics functions. This can be thought of as being inversely proportional to the overall "complexity" of the (set of) spherical harmonics function(s).

"power": this metric computes the sum of squared SH coefficients, which equals the mean-squared amplitude of the spherical function it represents.

The spherical harmonic coefficients are stored according to the conventions described in the main documentation, which can be found at the following link:  |br|
https://mrtrix.readthedocs.io/en/3.0.4/concepts/spherical_harmonics.html

Options
-------

Options specific to the "entropy" metric
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-normalise** normalise the voxel-wise entropy measure to the range [0.0, 1.0]

Options specific to the "power" metric
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-spectrum** output the power spectrum, i.e., the power contained within each harmonic degree (l=0, 2, 4, ...) as a 4D image.

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

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** J-Donald Tournier (jdtournier@gmail.com) and Robert E. Smith <robert.smith@florey.edu.au>

**Copyright:** Copyright (c) 2008-2024 the MRtrix3 contributors.

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


