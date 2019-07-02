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

Unary operators
^^^^^^^^^^^^^^^

-  **-abs**  *(multiple uses permitted)* absolute value

-  **-neg**  *(multiple uses permitted)* negative value

-  **-sqrt**  *(multiple uses permitted)* square root

-  **-exp**  *(multiple uses permitted)* exponential function

-  **-log**  *(multiple uses permitted)* natural logarithm

-  **-log10**  *(multiple uses permitted)* common logarithm

-  **-cos**  *(multiple uses permitted)* cosine

-  **-sin**  *(multiple uses permitted)* sine

-  **-tan**  *(multiple uses permitted)* tangent

-  **-cosh**  *(multiple uses permitted)* hyperbolic cosine

-  **-sinh**  *(multiple uses permitted)* hyperbolic sine

-  **-tanh**  *(multiple uses permitted)* hyperbolic tangent

-  **-acos**  *(multiple uses permitted)* inverse cosine

-  **-asin**  *(multiple uses permitted)* inverse sine

-  **-atan**  *(multiple uses permitted)* inverse tangent

-  **-acosh**  *(multiple uses permitted)* inverse hyperbolic cosine

-  **-asinh**  *(multiple uses permitted)* inverse hyperbolic sine

-  **-atanh**  *(multiple uses permitted)* inverse hyperbolic tangent

-  **-round**  *(multiple uses permitted)* round to nearest integer

-  **-ceil**  *(multiple uses permitted)* round up to nearest integer

-  **-floor**  *(multiple uses permitted)* round down to nearest integer

-  **-isnan**  *(multiple uses permitted)* true (1) is operand is not-a-number (NaN)

-  **-isinf**  *(multiple uses permitted)* true (1) is operand is infinite (Inf)

-  **-finite**  *(multiple uses permitted)* true (1) is operand is finite (i.e. not NaN or Inf)

-  **-real**  *(multiple uses permitted)* real part of complex number

-  **-imag**  *(multiple uses permitted)* imaginary part of complex number

-  **-phase**  *(multiple uses permitted)* phase of complex number

-  **-conj**  *(multiple uses permitted)* complex conjugate

Binary operators
^^^^^^^^^^^^^^^^

-  **-add**  *(multiple uses permitted)* add values

-  **-subtract**  *(multiple uses permitted)* subtract nth operand from (n-1)th

-  **-multiply**  *(multiple uses permitted)* multiply values

-  **-divide**  *(multiple uses permitted)* divide (n-1)th operand by nth

-  **-pow**  *(multiple uses permitted)* raise (n-1)th operand to nth power

-  **-min**  *(multiple uses permitted)* smallest of last two operands

-  **-max**  *(multiple uses permitted)* greatest of last two operands

-  **-lt**  *(multiple uses permitted)* less-than operator (true=1, false=0)

-  **-gt**  *(multiple uses permitted)* greater-than operator (true=1, false=0)

-  **-le**  *(multiple uses permitted)* less-than-or-equal-to operator (true=1, false=0)

-  **-ge**  *(multiple uses permitted)* greater-than-or-equal-to operator (true=1, false=0)

-  **-eq**  *(multiple uses permitted)* equal-to operator (true=1, false=0)

-  **-neq**  *(multiple uses permitted)* not-equal-to operator (true=1, false=0)

-  **-complex**  *(multiple uses permitted)* create complex number using the last two operands as real,imaginary components

Ternary operators
^^^^^^^^^^^^^^^^^

-  **-if**  *(multiple uses permitted)* if first operand is true (non-zero), return second operand, otherwise return third operand

-  **-replace**  *(multiple uses permitted)* Wherever first operand is equal to the second operand, replace with third operand

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


