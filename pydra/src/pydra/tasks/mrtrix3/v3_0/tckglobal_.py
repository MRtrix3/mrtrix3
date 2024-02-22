# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "source",
        ImageIn,
        {
            "argstr": "",
            "position": 0,
            "help_string": """the image containing the raw DWI data.""",
            "mandatory": True,
        },
    ),
    (
        "response",
        File,
        {
            "argstr": "",
            "position": 1,
            "help_string": """the response of a track segment on the DWI signal.""",
            "mandatory": True,
        },
    ),
    (
        "tracks",
        Tracks,
        {
            "argstr": "",
            "position": 2,
            "help_string": """the output file containing the tracks generated.""",
            "mandatory": True,
        },
    ),
    # Input options Option Group
    (
        "grad",
        File,
        {
            "argstr": "-grad",
            "help_string": """specify the diffusion encoding scheme (required if not supplied in the header).""",
        },
    ),
    (
        "mask",
        ImageIn,
        {
            "argstr": "-mask",
            "help_string": """only reconstruct the tractogram within the specified brain mask image.""",
        },
    ),
    (
        "riso",
        specs.MultiInputObj[File],
        {
            "argstr": "-riso",
            "help_string": """set one or more isotropic response functions. (multiple allowed)""",
        },
    ),
    # Parameters Option Group
    (
        "lmax",
        int,
        {
            "argstr": "-lmax",
            "help_string": """set the maximum harmonic order for the output series. (default = 8)""",
        },
    ),
    (
        "length",
        float,
        {
            "argstr": "-length",
            "help_string": """set the length of the particles (fibre segments). (default = 1mm)""",
        },
    ),
    (
        "weight",
        float,
        {
            "argstr": "-weight",
            "help_string": """set the weight by which particles contribute to the model. (default = 0.1)""",
        },
    ),
    (
        "ppot",
        float,
        {
            "argstr": "-ppot",
            "help_string": """set the particle potential, i.e., the cost of adding one segment, relative to the particle weight. (default = 0.05)""",
        },
    ),
    (
        "cpot",
        float,
        {
            "argstr": "-cpot",
            "help_string": """set the connection potential, i.e., the energy term that drives two segments together. (default = 0.5)""",
        },
    ),
    (
        "t0",
        float,
        {
            "argstr": "-t0",
            "help_string": """set the initial temperature of the metropolis hastings optimizer. (default = 0.1)""",
        },
    ),
    (
        "t1",
        float,
        {
            "argstr": "-t1",
            "help_string": """set the final temperature of the metropolis hastings optimizer. (default = 0.001)""",
        },
    ),
    (
        "niter",
        int,
        {
            "argstr": "-niter",
            "help_string": """set the number of iterations of the metropolis hastings optimizer. (default = 10M)""",
        },
    ),
    # Output options Option Group
    (
        "fod",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-fod",
            "output_file_template": "fod.mif",
            "help_string": """Predicted fibre orientation distribution function (fODF).
This fODF is estimated as part of the global track optimization, and therefore incorporates the spatial regularization that it imposes. Internally, the fODF is represented as a discrete sum of apodized point spread functions (aPSF) oriented along the directions of all particles in the voxel, used to predict the DWI signal from the particle configuration.""",
        },
    ),
    (
        "noapo",
        bool,
        {
            "argstr": "-noapo",
            "help_string": """disable spherical convolution of fODF with apodized PSF, to output a sum of delta functions rather than a sum of aPSFs.""",
        },
    ),
    (
        "fiso",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-fiso",
            "output_file_template": "fiso.mif",
            "help_string": """Predicted isotropic fractions of the tissues for which response functions were provided with -riso. Typically, these are CSF and GM.""",
        },
    ),
    (
        "eext",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-eext",
            "output_file_template": "eext.mif",
            "help_string": """Residual external energy in every voxel.""",
        },
    ),
    (
        "etrend",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-etrend",
            "output_file_template": "etrend.txt",
            "help_string": """internal and external energy trend and cooling statistics.""",
        },
    ),
    # Advanced parameters, if you really know what you're doing Option Group
    (
        "balance",
        float,
        {
            "argstr": "-balance",
            "help_string": """balance internal and external energy. (default = 0)
Negative values give more weight to the internal energy, positive to the external energy.""",
        },
    ),
    (
        "density",
        float,
        {
            "argstr": "-density",
            "help_string": """set the desired density of the free Poisson process. (default = 1)""",
        },
    ),
    (
        "prob",
        ty.List[float],
        {
            "argstr": "-prob",
            "help_string": """set the probabilities of generating birth, death, randshift, optshift and connect proposals respectively. (default = 0.25,0.05,0.25,0.1,0.35)""",
            "sep": ",",
        },
    ),
    (
        "beta",
        float,
        {
            "argstr": "-beta",
            "help_string": """set the width of the Hanning interpolation window. (in [0, 1], default = 0)
If used, a mask is required, and this mask must keep at least one voxel distance to the image bounding box.""",
        },
    ),
    (
        "lambda_",
        float,
        {
            "argstr": "-lambda",
            "help_string": """set the weight of the internal energy directly. (default = 1)
If provided, any value of -balance will be ignored.""",
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

tckglobal_input_spec = specs.SpecInfo(
    name="tckglobal_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "fod",
        ImageOut,
        {
            "help_string": """Predicted fibre orientation distribution function (fODF).
This fODF is estimated as part of the global track optimization, and therefore incorporates the spatial regularization that it imposes. Internally, the fODF is represented as a discrete sum of apodized point spread functions (aPSF) oriented along the directions of all particles in the voxel, used to predict the DWI signal from the particle configuration.""",
        },
    ),
    (
        "fiso",
        ImageOut,
        {
            "help_string": """Predicted isotropic fractions of the tissues for which response functions were provided with -riso. Typically, these are CSF and GM.""",
        },
    ),
    (
        "eext",
        ImageOut,
        {
            "help_string": """Residual external energy in every voxel.""",
        },
    ),
    (
        "etrend",
        File,
        {
            "help_string": """internal and external energy trend and cooling statistics.""",
        },
    ),
]
tckglobal_output_spec = specs.SpecInfo(
    name="tckglobal_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class tckglobal(ShellCommandTask):
    """This command will reconstruct the global white matter fibre tractogram that best explains the input DWI data, using a multi-tissue spherical convolution model.

        A more thorough description of the operation of global tractography in MRtrix3 can be found in the online documentation:
    https://mrtrix.readthedocs.io/en/3.0.4/quantitative_structural_connectivity/global_tractography.html


        Example usages
        --------------


        Basic usage:

            `$ tckglobal dwi.mif wmr.txt -riso csfr.txt -riso gmr.txt -mask mask.mif -niter 1e9 -fod fod.mif -fiso fiso.mif tracks.tck`

            dwi.mif is the input image, wmr.txt is an anisotropic, multi-shell response function for WM, and csfr.txt and gmr.txt are isotropic response functions for CSF and GM. The output tractogram is saved to tracks.tck. Optional output images fod.mif and fiso.mif contain the predicted WM fODF and isotropic tissue fractions of CSF and GM respectively, estimated as part of the global optimization and thus affected by spatial regularization.


        References
        ----------

            Christiaens, D.; Reisert, M.; Dhollander, T.; Sunaert, S.; Suetens, P. & Maes, F. Global tractography of multi-shell diffusion-weighted imaging data using a multi-tissue model. NeuroImage, 2015, 123, 89-101

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: Daan Christiaens (daan.christiaens@kcl.ac.uk)

            Copyright: Copyright (C) 2015 KU Leuven, Dept. Electrical Engineering, ESAT/PSI,
    Herestraat 49 box 7003, 3000 Leuven, Belgium

    This is free software; see the source for copying conditions.
    There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE."""

    executable = "tckglobal"
    input_spec = tckglobal_input_spec
    output_spec = tckglobal_output_spec
