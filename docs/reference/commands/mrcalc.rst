.. _mrcalc:

mrcalc
===================

Synopsis
--------

Apply generic voxel-wise mathematical operations to images

Usage
--------

::

    mrcalc [ options ]  operand [ operand ... ]

-  *operand*: an input image, intensity value, or the special keywords 'rand' (random number between 0 and 1) or 'randn' (random number from unit std.dev. normal distribution).

Description
-----------

This command will only compute per-voxel operations. Use 'mrmath' to compute summary statistics across images or along image axes.

This command uses a stack-based syntax, with operators (specified using options) operating on the top-most entries (i.e. images or values) in the stack. Operands (values or images) are pushed onto the stack in the order they appear (as arguments) on the command-line, and operators (specified as options) operate on and consume the top-most entries in the stack, and push their output as a new entry on the stack. For example:

    $ mrcalc a.mif 2 -mult r.mif

performs the operation r = 2*a for every voxel a,r in images a.mif and r.mif respectively. Similarly:

    $ mrcalc a.mif -neg b.mif -div -exp 9.3 -mult r.mif

performs the operation r = 9.3*exp(-a/b), and:

    $ mrcalc a.mif b.mif -add c.mif d.mif -mult 4.2 -add -div r.mif

performs r = (a+b)/(c*d+4.2).

As an additional feature, this command will allow images with different dimensions to be processed, provided they satisfy the following conditions: for each axis, the dimensions match if they are the same size, or one of them has size one. In the latter case, the entire image will be replicated along that axis. This allows for example a 4D image of size [ X Y Z N ] to be added to a 3D image of size [ X Y Z ], as if it consisted of N copies of the 3D image along the 4th axis (the missing dimension is assumed to have size 1). Another example would a single-voxel 4D image of size [ 1 1 1 N ], multiplied by a 3D image of size [ X Y Z ], which would allow the creation of a 4D image where each volume consists of the 3D image scaled by the corresponding value for that volume in the single-voxel image.

Options
-------

Unary operators
^^^^^^^^^^^^^^^

-  **-abs** absolute value

-  **-neg** negative value

-  **-sqrt** square root

-  **-exp** exponential function

-  **-log** natural logarithm

-  **-log10** common logarithm

-  **-cos** cosine

-  **-sin** sine

-  **-tan** tangent

-  **-cosh** hyperbolic cosine

-  **-sinh** hyperbolic sine

-  **-tanh** hyperbolic tangent

-  **-acos** inverse cosine

-  **-asin** inverse sine

-  **-atan** inverse tangent

-  **-acosh** inverse hyperbolic cosine

-  **-asinh** inverse hyperbolic sine

-  **-atanh** inverse hyperbolic tangent

-  **-round** round to nearest integer

-  **-ceil** round up to nearest integer

-  **-floor** round down to nearest integer

-  **-isnan** true (1) is operand is not-a-number (NaN)

-  **-isinf** true (1) is operand is infinite (Inf)

-  **-finite** true (1) is operand is finite (i.e. not NaN or Inf)

-  **-real** real part of complex number

-  **-imag** imaginary part of complex number

-  **-phase** phase of complex number

-  **-conj** complex conjugate

Binary operators
^^^^^^^^^^^^^^^^

-  **-add** add values

-  **-subtract** subtract nth operand from (n-1)th

-  **-multiply** multiply values

-  **-divide** divide (n-1)th operand by nth

-  **-pow** raise (n-1)th operand to nth power

-  **-min** smallest of last two operands

-  **-max** greatest of last two operands

-  **-lt** less-than operator (true=1, false=0)

-  **-gt** greater-than operator (true=1, false=0)

-  **-le** less-than-or-equal-to operator (true=1, false=0)

-  **-ge** greater-than-or-equal-to operator (true=1, false=0)

-  **-eq** equal-to operator (true=1, false=0)

-  **-neq** not-equal-to operator (true=1, false=0)

-  **-complex** create complex number using the last two operands as real,imaginary components

Ternary operators
^^^^^^^^^^^^^^^^^

-  **-if** if first operand is true (non-zero), return second operand, otherwise return third operand

Data type options
^^^^^^^^^^^^^^^^^

-  **-datatype spec** specify output image data type. Valid choices are: float32, float32le, float32be, float64, float64le, float64be, int64, uint64, int64le, uint64le, int64be, uint64be, int32, uint32, int32le, uint32le, int32be, uint32be, int16, uint16, int16le, uint16le, int16be, uint16be, cfloat32, cfloat32le, cfloat32be, cfloat64, cfloat64le, cfloat64be, int8, uint8, bit.

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

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


