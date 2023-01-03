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

The directions can be provided as: |br|
- a 2-column ASCII text file contained azimuth / elevation pairs (as produced by dirgen) |br|
- a 3-column ASCII text file containing x, y, z Cartesian direction vectors (as produced by dirgen -cart) |br|
- a 4-column ASCII text file containing the x, y, z, b components of a full DW encoding scheme (in MRtrix format, see main documentation for details). |br|
- an image file whose header contains a valid DW encoding scheme

If a full DW encoding is provided, the number of shells needs to match those found in the input image of coefficients (i.e. its size along the 5th dimension). If needed, the -shell option can be used to pick out the specific shell(s) of interest.

If the input image contains multiple shells (its size along the 5th dimension is greater than one), the program will expect the direction set to contain multiple shells, which can only be provided as a full DW encodings (the last two options in the list above).

The spherical harmonic coefficients are stored according the conventions described the main documentation, which can be found at the following link:  |br|
https://mrtrix.readthedocs.io/en/3.0.4/concepts/spherical_harmonics.html

Options
-------

-  **-nonnegative** cap all negative amplitudes to zero

DW gradient table import options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad file** Provide the diffusion-weighted gradient scheme used in the acquisition in a text file. This should be supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

-  **-fslgrad bvecs bvals** Provide the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format files. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

Stride options
^^^^^^^^^^^^^^

-  **-strides spec** specify the strides of the output data in memory; either as a comma-separated list of (signed) integers, or as a template image from which the strides shall be extracted and used. The actual strides produced will depend on whether the output image format can support it.

Data type options
^^^^^^^^^^^^^^^^^

-  **-datatype spec** specify output image data type. Valid choices are: float32, float32le, float32be, float64, float64le, float64be, int64, uint64, int64le, uint64le, int64be, uint64be, int32, uint32, int32le, uint32le, int32be, uint32be, int16, uint16, int16le, uint16le, int16be, uint16be, cfloat32, cfloat32le, cfloat32be, cfloat64, cfloat64le, cfloat64be, int8, uint8, bit.

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



**Author:** David Raffelt (david.raffelt@florey.edu.au) and J-Donald Tournier (jdtournier@gmail.com)

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


