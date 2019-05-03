.. _sh2amp:

sh2amp
===================

Synopsis
--------

Evaluate the amplitude of an image of spherical harmonic functions along specified directions

Usage
--------

::

    sh2amp [ options ]  input directions output

-  *input*: the input image consisting of spherical harmonic (SH) coefficients.
-  *directions*: the list of directions along which the SH functions will be sampled, generated using the dirgen command
-  *output*: the output image consisting of the amplitude of the SH functions along the specified directions.

Description
-----------

The input image should consist of a 4D or 5D image, with SH coefficients along the 4th dimension according to the convention below. If 4D (or size 1 along the 5th dimension), the program expects to be provided with a single shell of directions. If 5D, each set of coefficients along the 5th dimension is understood to correspond to a different shell.

The directions can be provided as:
- a 2-column ASCII text file contained azimuth / elevation pairs (as produced by dirgen)
- a 3-column ASCII text file containing x, y, z Cartesian direction vectors (as produced by dirgen -cart)
- a 4-column ASCII text file containing the x, y, z, b components of a full DW encoding scheme (in MRtrix format, see main documentation for details).
- an image file whose header contains a valid DW encoding scheme

If a full DW encoding is provided, the number of shells needs to match those found in the input image of coefficients (i.e. its size along the 5th dimension). If needed, the -shell option can be used to pick out the specific shell(s) of interest.

If the input image contains multiple shells (its size along the 5th dimension is greater than one), the program will expect the direction set to contain multiple shells, which can only be provided as a full DW encodings (the last two options in the list above).

The spherical harmonic coefficients are stored as follows. First, since the signal attenuation profile is real, it has conjugate symmetry, i.e. Y(l,-m) = Y(l,m)* (where * denotes the complex conjugate). Second, the diffusion profile should be antipodally symmetric (i.e. S(x) = S(-x)), implying that all odd l components should be zero. Therefore, only the even elements are computed.

Note that the spherical harmonics equations used here differ slightly from those conventionally used, in that the (-1)^m factor has been omitted. This should be taken into account in all subsequent calculations.

Each volume in the output image corresponds to a different spherical harmonic component. Each volume will correspond to the following:

volume 0: l = 0, m = 0
volume 1: l = 2, m = -2 (imaginary part of m=2 SH)
volume 2: l = 2, m = -1 (imaginary part of m=1 SH)
volume 3: l = 2, m = 0
volume 4: l = 2, m = 1 (real part of m=1 SH)
volume 5: l = 2, m = 2 (real part of m=2 SH)
etc...


Options
-------

-  **-nonnegative** cap all negative amplitudes to zero
   
DW shell selection options
^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-shells b-values** specify one or more b-values to use during processing, as a comma-separated list of the desired approximate b-values (b-values are clustered to allow for small deviations). Note that some commands are incompatible with multiple b-values, and will report an error if more than one b-value is provided. 
   WARNING: note that, even though the b=0 volumes are never referred to as shells in the literature, they still have to be explicitly included in the list of b-values as provided to the -shell option! Several algorithms which include the b=0 volumes in their computations may otherwise return an undesired result.
   
Stride options
^^^^^^^^^^^^^^

-  **-strides spec** specify the strides of the output data in memory; either as a comma-separated list of (signed) integers, or as a template image from which the strides shall be extracted and used. The actual strides produced will depend on whether the output image format can support it.
   
Data type options
^^^^^^^^^^^^^^^^^

-  **-datatype spec** specify output image data type. Valid choices are: float32, float32le, float32be, float64, float64le, float64be, int64, uint64, int64le, uint64le, int64be, uint64be, int32, uint32, int32le, uint32le, int32be, uint32be, int16, uint16, int16le, uint16le, int16be, uint16be, cfloat32, cfloat32le, cfloat32be, cfloat64, cfloat64le, cfloat64be, int8, uint8, bit.
   
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


