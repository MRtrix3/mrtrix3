import typing as ty
from pathlib import Path  # noqa: F401
from enum import Enum
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


class MrCalcOp(Enum):
    abs = "abs"  # return absolute value (magnitude) of real or complex number
    neg = "neg"  # negative value
    add = "add"  # add values
    subtract = "subtract"  # subtract values
    multiply = "multiply"  # multiply values
    divide = "divide"  # divide values
    modulo = "modulo"  # modulo operation
    min = "min"  # minimum value
    max = "max"  # maximum value
    lt = "lt"  # less than comparison
    gt = "gt"  # greater than comparison
    le = "le"  # less than or equal to comparison
    ge = "ge"  # greater than or equal to comparison
    eq = "eq"  # equal to comparison
    neq = "neq"  # not equal to comparison
    if_ = "if"  # if-else condition
    replace = "replace"  # replace values
    sqrt = "sqrt"  # square root
    pow = "pow"  # power operation
    round = "round"  # round to nearest integer
    ceil = "ceil"  # round up to nearest integer
    floor = "floor"  # round down to nearest integer
    not_ = "not"  # logical NOT operation
    and_ = "and"  # logical AND operation
    or_ = "or"  # logical OR operation
    xor = "xor"  # logical XOR operation
    isnan = "isnan"  # check if value is NaN
    isinf = "isinf"  # check if value is infinity
    finite = "finite"  # check if value is finite
    complex = "complex"  # convert to complex number
    polar = "polar"  # convert to polar coordinates
    real = "real"  # get real part of complex number
    imag = "imag"  # get imaginary part of complex number
    phase = "phase"  # get phase angle of complex number
    conj = "conj"  # complex conjugate
    proj = "proj"  # projection of complex number
    exp = "exp"  # exponential function
    log = "log"  # natural logarithm
    log10 = "log10"  # base-10 logarithm
    cos = "cos"  # cosine function
    sin = "sin"  # sine function
    tan = "tan"  # tangent function
    acos = "acos"  # arc cosine function
    asin = "asin"  # arc sine function
    atan = "atan"  # arc tangent function
    cosh = "cosh"  # hyperbolic cosine function
    sinh = "sinh"  # hyperbolic sine function
    tanh = "tanh"  # hyperbolic tangent function
    acosh = "acosh"  # inverse hyperbolic cosine function
    asinh = "asinh"  # inverse hyperbolic sine function
    atanh = "atanh"  # inverse hyperbolic tangent function


def operations_formatter(operations: ty.List[ty.Tuple[ty.List[ty.Union[ImageIn, float]], MrCalcOp]]):
    return " ".join([f"{' '.join([str(i) for i in operands])} -{op.value}" for operands, op in operations])


input_fields = [
    # Arguments
    (
        "operations",
        ty.List[ty.Tuple[ty.List[ty.Union[ImageIn, float]], MrCalcOp]],
        {
            "argstr": "",
            "sep": " ",
            "position": 0,
            "formatter": operations_formatter,
            "help_string": """an input image, intensity value, or the special keywords 'rand' (random number between 0 and 1) or 'randn' (random number from unit std.dev. normal distribution) or the mathematical constants 'e' and 'pi'.""",
            "mandatory": True,
        },
    ),
    (
        "output",
        Path,
        {
            "argstr": "",
            "position": -1,
            "help_string": """the output image.""",
            "output_file_template": "mrcalc_output.mif",
        },
    ),
    # Data type options Option Group
    (
        "datatype",
        str,
        {
            "argstr": "-datatype",
            "help_string": "specify output image data type.",
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


output_fields = [
    (
        "output",
        ImageOut,
        {
            "help_string": """the output image.""",
        },
    ),
]
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
    Operator = MrCalcOp

    executable = "mrcalc"
    input_spec = mrcalc_input_spec
    output_spec = mrcalc_output_spec
