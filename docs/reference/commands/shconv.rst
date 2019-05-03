.. _shconv:

shconv
===================

Synopsis
--------

Perform spherical convolution

Usage
--------

::

    shconv [ options ]  odf response [ odf response ... ] SH_out

-  *odf response*: pairs of input ODF image and corresponding responses
-  *SH_out*: the output spherical harmonics coefficients image.

Description
-----------

Provided with matching pairs of response function and ODF images (containing SH coefficients), perform spherical convolution to provide the corresponding SH coefficients of the signal.

If multiple pairs of inputs are provided, their contributions will be summed into a single output.

If the responses are multi-shell (with one line of coefficients per shell), the output will be a 5-dimensional image, with the SH coefficients of the signal in each shell stored at different indices along the 5th dimension.

The spherical harmonic coefficients are stored as follows. First, since the signal attenuation profile is real, it has conjugate symmetry, i.e. Y(l,-m) = Y(l,m)* (where * denotes the complex conjugate). Second, the diffusion profile should be antipodally symmetric (i.e. S(x) = S(-x)), implying that all odd l components should be zero. Therefore, only the even elements are computed.

Note that the spherical harmonics equations used here differ slightly from those conventionally used, in that the (-1)^m factor has been omitted. This should be taken into account in all subsequent calculations.

Each volume in the output image corresponds to a different spherical harmonic component. Each volume will correspond to the following:

volume 0: l = 0, m = 0 |br|
volume 1: l = 2, m = -2 (imaginary part of m=2 SH) |br|
volume 2: l = 2, m = -1 (imaginary part of m=1 SH) |br|
volume 3: l = 2, m = 0 |br|
volume 4: l = 2, m = 1 (real part of m=1 SH) |br|
volume 5: l = 2, m = 2 (real part of m=2 SH) |br|
etc... |br|


Options
-------

Data type options
^^^^^^^^^^^^^^^^^

-  **-datatype spec** specify output image data type. Valid choices are: float32, float32le, float32be, float64, float64le, float64be, int64, uint64, int64le, uint64le, int64be, uint64be, int32, uint32, int32le, uint32le, int32be, uint32be, int16, uint16, int16le, uint16le, int16be, uint16be, cfloat32, cfloat32le, cfloat32be, cfloat64, cfloat64le, cfloat64be, int8, uint8, bit.

Stride options
^^^^^^^^^^^^^^

-  **-strides spec** specify the strides of the output data in memory; either as a comma-separated list of (signed) integers, or as a template image from which the strides shall be extracted and used. The actual strides produced will depend on whether the output image format can support it.

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



**Author:** David Raffelt (david.raffelt@florey.edu.au)

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


