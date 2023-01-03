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

-  *operand*: an input image, intensity value, or the special keywords 'rand' (random number between 0 and 1) or 'randn' (random number from unit std.dev. normal distribution) or the mathematical constants 'e' and 'pi'.

Description
-----------

This command will only compute per-voxel operations. Use 'mrmath' to compute summary statistics across images or along image axes.

This command uses a stack-based syntax, with operators (specified using options) operating on the top-most entries (i.e. images or values) in the stack. Operands (values or images) are pushed onto the stack in the order they appear (as arguments) on the command-line, and operators (specified as options) operate on and consume the top-most entries in the stack, and push their output as a new entry on the stack.

As an additional feature, this command will allow images with different dimensions to be processed, provided they satisfy the following conditions: for each axis, the dimensions match if they are the same size, or one of them has size one. In the latter case, the entire image will be replicated along that axis. This allows for example a 4D image of size [ X Y Z N ] to be added to a 3D image of size [ X Y Z ], as if it consisted of N copies of the 3D image along the 4th axis (the missing dimension is assumed to have size 1). Another example would a single-voxel 4D image of size [ 1 1 1 N ], multiplied by a 3D image of size [ X Y Z ], which would allow the creation of a 4D image where each volume consists of the 3D image scaled by the corresponding value for that volume in the single-voxel image.

Example usages
--------------

-   *Double the value stored in every voxel*::

        $ mrcalc a.mif 2 -mult r.mif

    This performs the operation: r = 2*a  for every voxel a,r in images a.mif and r.mif respectively.

-   *A more complex example*::

        $ mrcalc a.mif -neg b.mif -div -exp 9.3 -mult r.mif

    This performs the operation: r = 9.3*exp(-a/b)

-   *Another complex example*::

        $ mrcalc a.mif b.mif -add c.mif d.mif -mult 4.2 -add -div r.mif

    This performs: r = (a+b)/(c*d+4.2).

-   *Rescale the densities in a SH l=0 image*::

        $ mrcalc ODF_CSF.mif 4 pi -mult -sqrt -div ODF_CSF_scaled.mif

    This applies the spherical harmonic basis scaling factor: 1.0/sqrt(4*pi), such that a single-tissue voxel containing the same intensities as the response function of that tissue should contain the value 1.0.

Options
-------

basic operations
^^^^^^^^^^^^^^^^

-  **-abs** *(multiple uses permitted)* \|%1\| : return absolute value (magnitude) of real or complex number

-  **-neg** *(multiple uses permitted)* -%1 : negative value

-  **-add** *(multiple uses permitted)* (%1 + %2) : add values

-  **-subtract** *(multiple uses permitted)* (%1 - %2) : subtract nth operand from (n-1)th

-  **-multiply** *(multiple uses permitted)* (%1 * %2) : multiply values

-  **-divide** *(multiple uses permitted)* (%1 / %2) : divide (n-1)th operand by nth

-  **-min** *(multiple uses permitted)* min (%1, %2) : smallest of last two operands

-  **-max** *(multiple uses permitted)* max (%1, %2) : greatest of last two operands

comparison operators
^^^^^^^^^^^^^^^^^^^^

-  **-lt** *(multiple uses permitted)* (%1 < %2) : less-than operator (true=1, false=0)

-  **-gt** *(multiple uses permitted)* (%1 > %2) : greater-than operator (true=1, false=0)

-  **-le** *(multiple uses permitted)* (%1 <= %2) : less-than-or-equal-to operator (true=1, false=0)

-  **-ge** *(multiple uses permitted)* (%1 >= %2) : greater-than-or-equal-to operator (true=1, false=0)

-  **-eq** *(multiple uses permitted)* (%1 == %2) : equal-to operator (true=1, false=0)

-  **-neq** *(multiple uses permitted)* (%1 != %2) : not-equal-to operator (true=1, false=0)

conditional operators
^^^^^^^^^^^^^^^^^^^^^

-  **-if** *(multiple uses permitted)* (%1 ? %2 : %3) : if first operand is true (non-zero), return second operand, otherwise return third operand

-  **-replace** *(multiple uses permitted)* (%1, %2 -> %3) : Wherever first operand is equal to the second operand, replace with third operand

power functions
^^^^^^^^^^^^^^^

-  **-sqrt** *(multiple uses permitted)* sqrt (%1) : square root

-  **-pow** *(multiple uses permitted)* %1^%2 : raise (n-1)th operand to nth power

nearest integer operations
^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-round** *(multiple uses permitted)* round (%1) : round to nearest integer

-  **-ceil** *(multiple uses permitted)* ceil (%1) : round up to nearest integer

-  **-floor** *(multiple uses permitted)* floor (%1) : round down to nearest integer

logical operators
^^^^^^^^^^^^^^^^^

-  **-not** *(multiple uses permitted)* !%1 : NOT operator: true (1) if operand is false (i.e. zero)

-  **-and** *(multiple uses permitted)* (%1 && %2) : AND operator: true (1) if both operands are true (i.e. non-zero)

-  **-or** *(multiple uses permitted)* (%1 \|\| %2) : OR operator: true (1) if either operand is true (i.e. non-zero)

-  **-xor** *(multiple uses permitted)* (%1 ^^ %2) : XOR operator: true (1) if only one of the operands is true (i.e. non-zero)

classification functions
^^^^^^^^^^^^^^^^^^^^^^^^

-  **-isnan** *(multiple uses permitted)* isnan (%1) : true (1) if operand is not-a-number (NaN)

-  **-isinf** *(multiple uses permitted)* isinf (%1) : true (1) if operand is infinite (Inf)

-  **-finite** *(multiple uses permitted)* finite (%1) : true (1) if operand is finite (i.e. not NaN or Inf)

complex numbers
^^^^^^^^^^^^^^^

-  **-complex** *(multiple uses permitted)* (%1 + %2 i) : create complex number using the last two operands as real,imaginary components

-  **-polar** *(multiple uses permitted)* (%1 /_ %2) : create complex number using the last two operands as magnitude,phase components (phase in radians)

-  **-real** *(multiple uses permitted)* real (%1) : real part of complex number

-  **-imag** *(multiple uses permitted)* imag (%1) : imaginary part of complex number

-  **-phase** *(multiple uses permitted)* phase (%1) : phase of complex number (use -abs for magnitude)

-  **-conj** *(multiple uses permitted)* conj (%1) : complex conjugate

-  **-proj** *(multiple uses permitted)* proj (%1) : projection onto the Riemann sphere

exponential functions
^^^^^^^^^^^^^^^^^^^^^

-  **-exp** *(multiple uses permitted)* exp (%1) : exponential function

-  **-log** *(multiple uses permitted)* log (%1) : natural logarithm

-  **-log10** *(multiple uses permitted)* log10 (%1) : common logarithm

trigonometric functions
^^^^^^^^^^^^^^^^^^^^^^^

-  **-cos** *(multiple uses permitted)* cos (%1) : cosine

-  **-sin** *(multiple uses permitted)* sin (%1) : sine

-  **-tan** *(multiple uses permitted)* tan (%1) : tangent

-  **-acos** *(multiple uses permitted)* acos (%1) : inverse cosine

-  **-asin** *(multiple uses permitted)* asin (%1) : inverse sine

-  **-atan** *(multiple uses permitted)* atan (%1) : inverse tangent

hyperbolic functions
^^^^^^^^^^^^^^^^^^^^

-  **-cosh** *(multiple uses permitted)* cosh (%1) : hyperbolic cosine

-  **-sinh** *(multiple uses permitted)* sinh (%1) : hyperbolic sine

-  **-tanh** *(multiple uses permitted)* tanh (%1) : hyperbolic tangent

-  **-acosh** *(multiple uses permitted)* acosh (%1) : inverse hyperbolic cosine

-  **-asinh** *(multiple uses permitted)* asinh (%1) : inverse hyperbolic sine

-  **-atanh** *(multiple uses permitted)* atanh (%1) : inverse hyperbolic tangent

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


