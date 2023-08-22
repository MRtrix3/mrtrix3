# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "operand",
        specs.MultiInputObj[ty.Any],
        {
            "argstr": "",
            "position": 0,
            "help_string": """an input image, intensity value, or the special keywords 'rand' (random number between 0 and 1) or 'randn' (random number from unit std.dev. normal distribution) or the mathematical constants 'e' and 'pi'.""",
            "mandatory": True,
        },
    ),
    # basic operations Option Group
    (
        "abs",
        specs.MultiInputObj[bool],
        {
            "argstr": "-abs",
            "help_string": """|%1| : return absolute value (magnitude) of real or complex number""",
        },
    ),
    (
        "neg",
        specs.MultiInputObj[bool],
        {
            "argstr": "-neg",
            "help_string": """-%1 : negative value""",
        },
    ),
    (
        "add",
        specs.MultiInputObj[bool],
        {
            "argstr": "-add",
            "help_string": """(%1 + %2) : add values""",
        },
    ),
    (
        "subtract",
        specs.MultiInputObj[bool],
        {
            "argstr": "-subtract",
            "help_string": """(%1 - %2) : subtract nth operand from (n-1)th""",
        },
    ),
    (
        "multiply",
        specs.MultiInputObj[bool],
        {
            "argstr": "-multiply",
            "help_string": """(%1 * %2) : multiply values""",
        },
    ),
    (
        "divide",
        specs.MultiInputObj[bool],
        {
            "argstr": "-divide",
            "help_string": """(%1 / %2) : divide (n-1)th operand by nth""",
        },
    ),
    (
        "modulo",
        specs.MultiInputObj[bool],
        {
            "argstr": "-modulo",
            "help_string": """(%1 % %2) : remainder after dividing (n-1)th operand by nth""",
        },
    ),
    (
        "min",
        specs.MultiInputObj[bool],
        {
            "argstr": "-min",
            "help_string": """min (%1, %2) : smallest of last two operands""",
        },
    ),
    (
        "max",
        specs.MultiInputObj[bool],
        {
            "argstr": "-max",
            "help_string": """max (%1, %2) : greatest of last two operands""",
        },
    ),
    # comparison operators Option Group
    (
        "lt",
        specs.MultiInputObj[bool],
        {
            "argstr": "-lt",
            "help_string": """(%1 < %2) : less-than operator (true=1, false=0)""",
        },
    ),
    (
        "gt",
        specs.MultiInputObj[bool],
        {
            "argstr": "-gt",
            "help_string": """(%1 > %2) : greater-than operator (true=1, false=0)""",
        },
    ),
    (
        "le",
        specs.MultiInputObj[bool],
        {
            "argstr": "-le",
            "help_string": """(%1 <= %2) : less-than-or-equal-to operator (true=1, false=0)""",
        },
    ),
    (
        "ge",
        specs.MultiInputObj[bool],
        {
            "argstr": "-ge",
            "help_string": """(%1 >= %2) : greater-than-or-equal-to operator (true=1, false=0)""",
        },
    ),
    (
        "eq",
        specs.MultiInputObj[bool],
        {
            "argstr": "-eq",
            "help_string": """(%1 == %2) : equal-to operator (true=1, false=0)""",
        },
    ),
    (
        "neq",
        specs.MultiInputObj[bool],
        {
            "argstr": "-neq",
            "help_string": """(%1 != %2) : not-equal-to operator (true=1, false=0)""",
        },
    ),
    # conditional operators Option Group
    (
        "if_",
        specs.MultiInputObj[bool],
        {
            "argstr": "-if",
            "help_string": """(%1 ? %2 : %3) : if first operand is true (non-zero), return second operand, otherwise return third operand""",
        },
    ),
    (
        "replace",
        specs.MultiInputObj[bool],
        {
            "argstr": "-replace",
            "help_string": """(%1, %2 -> %3) : Wherever first operand is equal to the second operand, replace with third operand""",
        },
    ),
    # power functions Option Group
    (
        "sqrt",
        specs.MultiInputObj[bool],
        {
            "argstr": "-sqrt",
            "help_string": """sqrt (%1) : square root""",
        },
    ),
    (
        "pow",
        specs.MultiInputObj[bool],
        {
            "argstr": "-pow",
            "help_string": """%1^%2 : raise (n-1)th operand to nth power""",
        },
    ),
    # nearest integer operations Option Group
    (
        "round",
        specs.MultiInputObj[bool],
        {
            "argstr": "-round",
            "help_string": """round (%1) : round to nearest integer""",
        },
    ),
    (
        "ceil",
        specs.MultiInputObj[bool],
        {
            "argstr": "-ceil",
            "help_string": """ceil (%1) : round up to nearest integer""",
        },
    ),
    (
        "floor",
        specs.MultiInputObj[bool],
        {
            "argstr": "-floor",
            "help_string": """floor (%1) : round down to nearest integer""",
        },
    ),
    # logical operators Option Group
    (
        "not_",
        specs.MultiInputObj[bool],
        {
            "argstr": "-not",
            "help_string": """!%1 : NOT operator: true (1) if operand is false (i.e. zero)""",
        },
    ),
    (
        "and_",
        specs.MultiInputObj[bool],
        {
            "argstr": "-and",
            "help_string": """(%1 && %2) : AND operator: true (1) if both operands are true (i.e. non-zero)""",
        },
    ),
    (
        "or_",
        specs.MultiInputObj[bool],
        {
            "argstr": "-or",
            "help_string": """(%1 || %2) : OR operator: true (1) if either operand is true (i.e. non-zero)""",
        },
    ),
    (
        "xor",
        specs.MultiInputObj[bool],
        {
            "argstr": "-xor",
            "help_string": """(%1 ^^ %2) : XOR operator: true (1) if only one of the operands is true (i.e. non-zero)""",
        },
    ),
    # classification functions Option Group
    (
        "isnan",
        specs.MultiInputObj[bool],
        {
            "argstr": "-isnan",
            "help_string": """isnan (%1) : true (1) if operand is not-a-number (NaN)""",
        },
    ),
    (
        "isinf",
        specs.MultiInputObj[bool],
        {
            "argstr": "-isinf",
            "help_string": """isinf (%1) : true (1) if operand is infinite (Inf)""",
        },
    ),
    (
        "finite",
        specs.MultiInputObj[bool],
        {
            "argstr": "-finite",
            "help_string": """finite (%1) : true (1) if operand is finite (i.e. not NaN or Inf)""",
        },
    ),
    # complex numbers Option Group
    (
        "complex",
        specs.MultiInputObj[bool],
        {
            "argstr": "-complex",
            "help_string": """(%1 + %2 i) : create complex number using the last two operands as real,imaginary components""",
        },
    ),
    (
        "polar",
        specs.MultiInputObj[bool],
        {
            "argstr": "-polar",
            "help_string": """(%1 /_ %2) : create complex number using the last two operands as magnitude,phase components (phase in radians)""",
        },
    ),
    (
        "real",
        specs.MultiInputObj[bool],
        {
            "argstr": "-real",
            "help_string": """real (%1) : real part of complex number""",
        },
    ),
    (
        "imag",
        specs.MultiInputObj[bool],
        {
            "argstr": "-imag",
            "help_string": """imag (%1) : imaginary part of complex number""",
        },
    ),
    (
        "phase",
        specs.MultiInputObj[bool],
        {
            "argstr": "-phase",
            "help_string": """phase (%1) : phase of complex number (use -abs for magnitude)""",
        },
    ),
    (
        "conj",
        specs.MultiInputObj[bool],
        {
            "argstr": "-conj",
            "help_string": """conj (%1) : complex conjugate""",
        },
    ),
    (
        "proj",
        specs.MultiInputObj[bool],
        {
            "argstr": "-proj",
            "help_string": """proj (%1) : projection onto the Riemann sphere""",
        },
    ),
    # exponential functions Option Group
    (
        "exp",
        specs.MultiInputObj[bool],
        {
            "argstr": "-exp",
            "help_string": """exp (%1) : exponential function""",
        },
    ),
    (
        "log",
        specs.MultiInputObj[bool],
        {
            "argstr": "-log",
            "help_string": """log (%1) : natural logarithm""",
        },
    ),
    (
        "log10",
        specs.MultiInputObj[bool],
        {
            "argstr": "-log10",
            "help_string": """log10 (%1) : common logarithm""",
        },
    ),
    # trigonometric functions Option Group
    (
        "cos",
        specs.MultiInputObj[bool],
        {
            "argstr": "-cos",
            "help_string": """cos (%1) : cosine""",
        },
    ),
    (
        "sin",
        specs.MultiInputObj[bool],
        {
            "argstr": "-sin",
            "help_string": """sin (%1) : sine""",
        },
    ),
    (
        "tan",
        specs.MultiInputObj[bool],
        {
            "argstr": "-tan",
            "help_string": """tan (%1) : tangent""",
        },
    ),
    (
        "acos",
        specs.MultiInputObj[bool],
        {
            "argstr": "-acos",
            "help_string": """acos (%1) : inverse cosine""",
        },
    ),
    (
        "asin",
        specs.MultiInputObj[bool],
        {
            "argstr": "-asin",
            "help_string": """asin (%1) : inverse sine""",
        },
    ),
    (
        "atan",
        specs.MultiInputObj[bool],
        {
            "argstr": "-atan",
            "help_string": """atan (%1) : inverse tangent""",
        },
    ),
    # hyperbolic functions Option Group
    (
        "cosh",
        specs.MultiInputObj[bool],
        {
            "argstr": "-cosh",
            "help_string": """cosh (%1) : hyperbolic cosine""",
        },
    ),
    (
        "sinh",
        specs.MultiInputObj[bool],
        {
            "argstr": "-sinh",
            "help_string": """sinh (%1) : hyperbolic sine""",
        },
    ),
    (
        "tanh",
        specs.MultiInputObj[bool],
        {
            "argstr": "-tanh",
            "help_string": """tanh (%1) : hyperbolic tangent""",
        },
    ),
    (
        "acosh",
        specs.MultiInputObj[bool],
        {
            "argstr": "-acosh",
            "help_string": """acosh (%1) : inverse hyperbolic cosine""",
        },
    ),
    (
        "asinh",
        specs.MultiInputObj[bool],
        {
            "argstr": "-asinh",
            "help_string": """asinh (%1) : inverse hyperbolic sine""",
        },
    ),
    (
        "atanh",
        specs.MultiInputObj[bool],
        {
            "argstr": "-atanh",
            "help_string": """atanh (%1) : inverse hyperbolic tangent""",
        },
    ),
    # Data type options Option Group
    (
        "datatype",
        str,
        {
            "argstr": "-datatype",
            "help_string": """specify output image data type. Valid choices are: float16, float16le, float16be, float32, float32le, float32be, float64, float64le, float64be, int64, uint64, int64le, uint64le, int64be, uint64be, int32, uint32, int32le, uint32le, int32be, uint32be, int16, uint16, int16le, uint16le, int16be, uint16be, cfloat16, cfloat16le, cfloat16be, cfloat32, cfloat32le, cfloat32be, cfloat64, cfloat64le, cfloat64be, int8, uint8, bit.""",
            "allowed_values": [
                "float16",
                "float16",
                "float16le",
                "float16be",
                "float32",
                "float32le",
                "float32be",
                "float64",
                "float64le",
                "float64be",
                "int64",
                "uint64",
                "int64le",
                "uint64le",
                "int64be",
                "uint64be",
                "int32",
                "uint32",
                "int32le",
                "uint32le",
                "int32be",
                "uint32be",
                "int16",
                "uint16",
                "int16le",
                "uint16le",
                "int16be",
                "uint16be",
                "cfloat16",
                "cfloat16le",
                "cfloat16be",
                "cfloat32",
                "cfloat32le",
                "cfloat32be",
                "cfloat64",
                "cfloat64le",
                "cfloat64be",
                "int8",
                "uint8",
                "bit",
            ],
        },
    ),
    # Standard options
    (
        "info",
        bool,
        {
            "argstr": "-info",
            "help_string": """display information messages.""",
        },
    ),
    (
        "quiet",
        bool,
        {
            "argstr": "-quiet",
            "help_string": """do not display information messages or progress status; alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.""",
        },
    ),
    (
        "debug",
        bool,
        {
            "argstr": "-debug",
            "help_string": """display debugging messages.""",
        },
    ),
    (
        "force",
        bool,
        {
            "argstr": "-force",
            "help_string": """force overwrite of output files (caution: using the same file as input and output might cause unexpected behaviour).""",
        },
    ),
    (
        "nthreads",
        int,
        {
            "argstr": "-nthreads",
            "help_string": """use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).""",
        },
    ),
    (
        "config",
        specs.MultiInputObj[ty.Tuple[str, str]],
        {
            "argstr": "-config",
            "help_string": """temporarily set the value of an MRtrix config file entry.""",
        },
    ),
    (
        "help",
        bool,
        {
            "argstr": "-help",
            "help_string": """display this information page and exit.""",
        },
    ),
    (
        "version",
        bool,
        {
            "argstr": "-version",
            "help_string": """display version information and exit.""",
        },
    ),
]

mrcalc_input_spec = specs.SpecInfo(
    name="mrcalc_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = []
mrcalc_output_spec = specs.SpecInfo(
    name="mrcalc_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class mrcalc(ShellCommandTask):
    """This command will only compute per-voxel operations. Use 'mrmath' to compute summary statistics across images or along image axes.

        This command uses a stack-based syntax, with operators (specified using options) operating on the top-most entries (i.e. images or values) in the stack. Operands (values or images) are pushed onto the stack in the order they appear (as arguments) on the command-line, and operators (specified as options) operate on and consume the top-most entries in the stack, and push their output as a new entry on the stack.

        As an additional feature, this command will allow images with different dimensions to be processed, provided they satisfy the following conditions: for each axis, the dimensions match if they are the same size, or one of them has size one. In the latter case, the entire image will be replicated along that axis. This allows for example a 4D image of size [ X Y Z N ] to be added to a 3D image of size [ X Y Z ], as if it consisted of N copies of the 3D image along the 4th axis (the missing dimension is assumed to have size 1). Another example would a single-voxel 4D image of size [ 1 1 1 N ], multiplied by a 3D image of size [ X Y Z ], which would allow the creation of a 4D image where each volume consists of the 3D image scaled by the corresponding value for that volume in the single-voxel image.


        Example usages
        --------------


        Double the value stored in every voxel:

            `$ mrcalc a.mif 2 -mult r.mif`

            This performs the operation: r = 2*a  for every voxel a,r in images a.mif and r.mif respectively.


        A more complex example:

            `$ mrcalc a.mif -neg b.mif -div -exp 9.3 -mult r.mif`

            This performs the operation: r = 9.3*exp(-a/b)


        Another complex example:

            `$ mrcalc a.mif b.mif -add c.mif d.mif -mult 4.2 -add -div r.mif`

            This performs: r = (a+b)/(c*d+4.2).


        Rescale the densities in a SH l=0 image:

            `$ mrcalc ODF_CSF.mif 4 pi -mult -sqrt -div ODF_CSF_scaled.mif`

            This applies the spherical harmonic basis scaling factor: 1.0/sqrt(4*pi), such that a single-tissue voxel containing the same intensities as the response function of that tissue should contain the value 1.0.


        References
        ----------

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: J-Donald Tournier (jdtournier@gmail.com)

            Copyright: Copyright (c) 2008-2023 the MRtrix3 contributors.

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
    """

    executable = "mrcalc"
    input_spec = mrcalc_input_spec
    output_spec = mrcalc_output_spec
