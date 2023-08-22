# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "image1_image2",
        ImageIn,
        {
            "argstr": "",
            "position": 0,
            "help_string": """input image 1 ('moving') and input image 2 ('template')""",
            "mandatory": True,
        },
    ),
    (
        "contrast1_contrast2",
        specs.MultiInputObj[ImageIn],
        {
            "argstr": "",
            "position": 1,
            "help_string": """optional list of additional input images used as additional contrasts. Can be used multiple times. contrastX and imageX must share the same coordinate system. """,
        },
    ),
    (
        "type",
        str,
        {
            "argstr": "-type",
            "help_string": """the registration type. Valid choices are: rigid, affine, nonlinear, rigid_affine, rigid_nonlinear, affine_nonlinear, rigid_affine_nonlinear (Default: affine_nonlinear)""",
            "allowed_values": [
                "rigid",
                "rigid",
                "affine",
                "nonlinear",
                "rigid_affine",
                "rigid_nonlinear",
                "affine_nonlinear",
                "rigid_affine_nonlinear",
            ],
        },
    ),
    (
        "transformed",
        specs.MultiInputObj[Path],
        {
            "argstr": "-transformed",
            "output_file_template": "transformed.mif",
            "help_string": """image1 after registration transformed and regridded to the space of image2. Note that -transformed needs to be repeated for each contrast if multi-contrast registration is used.""",
        },
    ),
    (
        "transformed_midway",
        specs.MultiInputObj[ty.Tuple[Path, Path]],
        {
            "argstr": "-transformed_midway",
            "output_file_template": (
                "transformed_midway0.mif",
                "transformed_midway1.mif",
            ),
            "help_string": """image1 and image2 after registration transformed and regridded to the midway space. Note that -transformed_midway needs to be repeated for each contrast if multi-contrast registration is used.""",
        },
    ),
    (
        "mask1",
        ImageIn,
        {
            "argstr": "-mask1",
            "help_string": """a mask to define the region of image1 to use for optimisation.""",
        },
    ),
    (
        "mask2",
        ImageIn,
        {
            "argstr": "-mask2",
            "help_string": """a mask to define the region of image2 to use for optimisation.""",
        },
    ),
    (
        "nan",
        bool,
        {
            "argstr": "-nan",
            "help_string": """use NaN as out of bounds value. (Default: 0.0)""",
        },
    ),
    # Rigid registration options Option Group
    (
        "rigid",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-rigid",
            "output_file_template": "rigid.txt",
            "help_string": """the output text file containing the rigid transformation as a 4x4 matrix""",
        },
    ),
    (
        "rigid_1tomidway",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-rigid_1tomidway",
            "output_file_template": "rigid_1tomidway.txt",
            "help_string": """the output text file containing the rigid transformation that aligns image1 to image2 in their common midway space as a 4x4 matrix""",
        },
    ),
    (
        "rigid_2tomidway",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-rigid_2tomidway",
            "output_file_template": "rigid_2tomidway.txt",
            "help_string": """the output text file containing the rigid transformation that aligns image2 to image1 in their common midway space as a 4x4 matrix""",
        },
    ),
    (
        "rigid_init_translation",
        str,
        {
            "argstr": "-rigid_init_translation",
            "help_string": """initialise the translation and centre of rotation 
Valid choices are: 
mass (aligns the centers of mass of both images, default), 
geometric (aligns geometric image centres) and none.""",
            "allowed_values": ["mass", "mass", "geometric", "none"],
        },
    ),
    (
        "rigid_init_rotation",
        str,
        {
            "argstr": "-rigid_init_rotation",
            "help_string": """initialise the rotation Valid choices are: 
search (search for the best rotation using mean squared residuals), 
moments (rotation based on directions of intensity variance with respect to centre of mass), 
none (default).""",
            "allowed_values": ["search", "search", "moments", "none"],
        },
    ),
    (
        "rigid_init_matrix",
        File,
        {
            "argstr": "-rigid_init_matrix",
            "help_string": """initialise either the rigid, affine, or syn registration with the supplied rigid transformation (as a 4x4 matrix in scanner coordinates). Note that this overrides rigid_init_translation and rigid_init_rotation initialisation """,
        },
    ),
    (
        "rigid_scale",
        ty.List[float],
        {
            "argstr": "-rigid_scale",
            "help_string": """use a multi-resolution scheme by defining a scale factor for each level using comma separated values (Default: 0.25,0.5,1.0)""",
            "sep": ",",
        },
    ),
    (
        "rigid_niter",
        ty.List[int],
        {
            "argstr": "-rigid_niter",
            "help_string": """the maximum number of gradient descent iterations per stage. This can be specified either as a single number for all multi-resolution levels, or a single value for each level. (Default: 1000)""",
            "sep": ",",
        },
    ),
    (
        "rigid_metric",
        str,
        {
            "argstr": "-rigid_metric",
            "help_string": """valid choices are: diff (intensity differences), Default: diff""",
            "allowed_values": ["diff", "diff", "ncc"],
        },
    ),
    (
        "rigid_metric_diff_estimator",
        str,
        {
            "argstr": "-rigid_metric.diff.estimator",
            "help_string": """Valid choices are: l1 (least absolute: |x|), l2 (ordinary least squares), lp (least powers: |x|^1.2), Default: l2""",
            "allowed_values": ["l1", "l1", "l2", "lp"],
        },
    ),
    (
        "rigid_lmax",
        ty.List[int],
        {
            "argstr": "-rigid_lmax",
            "help_string": """explicitly set the lmax to be used per scale factor in rigid FOD registration. By default FOD registration will use lmax 0,2,4 with default scale factors 0.25,0.5,1.0 respectively. Note that no reorientation will be performed with lmax = 0.""",
            "sep": ",",
        },
    ),
    (
        "rigid_log",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-rigid_log",
            "output_file_template": "rigid_log.txt",
            "help_string": """write gradient descent parameter evolution to log file""",
        },
    ),
    # Affine registration options Option Group
    (
        "affine",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-affine",
            "output_file_template": "affine.txt",
            "help_string": """the output text file containing the affine transformation as a 4x4 matrix""",
        },
    ),
    (
        "affine_1tomidway",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-affine_1tomidway",
            "output_file_template": "affine_1tomidway.txt",
            "help_string": """the output text file containing the affine transformation that aligns image1 to image2 in their common midway space as a 4x4 matrix""",
        },
    ),
    (
        "affine_2tomidway",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-affine_2tomidway",
            "output_file_template": "affine_2tomidway.txt",
            "help_string": """the output text file containing the affine transformation that aligns image2 to image1 in their common midway space as a 4x4 matrix""",
        },
    ),
    (
        "affine_init_translation",
        str,
        {
            "argstr": "-affine_init_translation",
            "help_string": """initialise the translation and centre of rotation 
Valid choices are: 
mass (aligns the centers of mass of both images), 
geometric (aligns geometric image centres) and none. (Default: mass)""",
            "allowed_values": ["mass", "mass", "geometric", "none"],
        },
    ),
    (
        "affine_init_rotation",
        str,
        {
            "argstr": "-affine_init_rotation",
            "help_string": """initialise the rotation Valid choices are: 
search (search for the best rotation using mean squared residuals), 
moments (rotation based on directions of intensity variance with respect to centre of mass), 
none (Default: none).""",
            "allowed_values": ["search", "search", "moments", "none"],
        },
    ),
    (
        "affine_init_matrix",
        File,
        {
            "argstr": "-affine_init_matrix",
            "help_string": """initialise either the affine, or syn registration with the supplied affine transformation (as a 4x4 matrix in scanner coordinates). Note that this overrides affine_init_translation and affine_init_rotation initialisation """,
        },
    ),
    (
        "affine_scale",
        ty.List[float],
        {
            "argstr": "-affine_scale",
            "help_string": """use a multi-resolution scheme by defining a scale factor for each level using comma separated values (Default: 0.25,0.5,1.0)""",
            "sep": ",",
        },
    ),
    (
        "affine_niter",
        ty.List[int],
        {
            "argstr": "-affine_niter",
            "help_string": """the maximum number of gradient descent iterations per stage. This can be specified either as a single number for all multi-resolution levels, or a single value for each level. (Default: 1000)""",
            "sep": ",",
        },
    ),
    (
        "affine_metric",
        str,
        {
            "argstr": "-affine_metric",
            "help_string": """valid choices are: diff (intensity differences), Default: diff""",
            "allowed_values": ["diff", "diff", "ncc"],
        },
    ),
    (
        "affine_metric_diff_estimator",
        str,
        {
            "argstr": "-affine_metric.diff.estimator",
            "help_string": """Valid choices are: l1 (least absolute: |x|), l2 (ordinary least squares), lp (least powers: |x|^1.2), Default: l2""",
            "allowed_values": ["l1", "l1", "l2", "lp"],
        },
    ),
    (
        "affine_lmax",
        ty.List[int],
        {
            "argstr": "-affine_lmax",
            "help_string": """explicitly set the lmax to be used per scale factor in affine FOD registration. By default FOD registration will use lmax 0,2,4 with default scale factors 0.25,0.5,1.0 respectively. Note that no reorientation will be performed with lmax = 0.""",
            "sep": ",",
        },
    ),
    (
        "affine_log",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-affine_log",
            "output_file_template": "affine_log.txt",
            "help_string": """write gradient descent parameter evolution to log file""",
        },
    ),
    # Advanced linear transformation initialisation options Option Group
    (
        "init_translation_unmasked1",
        bool,
        {
            "argstr": "-init_translation.unmasked1",
            "help_string": """disregard mask1 for the translation initialisation (affects 'mass')""",
        },
    ),
    (
        "init_translation_unmasked2",
        bool,
        {
            "argstr": "-init_translation.unmasked2",
            "help_string": """disregard mask2 for the translation initialisation (affects 'mass')""",
        },
    ),
    (
        "init_rotation_unmasked1",
        bool,
        {
            "argstr": "-init_rotation.unmasked1",
            "help_string": """disregard mask1 for the rotation initialisation (affects 'search' and 'moments')""",
        },
    ),
    (
        "init_rotation_unmasked2",
        bool,
        {
            "argstr": "-init_rotation.unmasked2",
            "help_string": """disregard mask2 for the rotation initialisation (affects 'search' and 'moments')""",
        },
    ),
    (
        "init_rotation_search_angles",
        ty.List[float],
        {
            "argstr": "-init_rotation.search.angles",
            "help_string": """rotation angles for the local search in degrees between 0 and 180. (Default: 2,5,10,15,20)""",
            "sep": ",",
        },
    ),
    (
        "init_rotation_search_scale",
        float,
        {
            "argstr": "-init_rotation.search.scale",
            "help_string": """relative size of the images used for the rotation search. (Default: 0.15)""",
        },
    ),
    (
        "init_rotation_search_directions",
        int,
        {
            "argstr": "-init_rotation.search.directions",
            "help_string": """number of rotation axis for local search. (Default: 250)""",
        },
    ),
    (
        "init_rotation_search_run_global",
        bool,
        {
            "argstr": "-init_rotation.search.run_global",
            "help_string": """perform a global search. (Default: local)""",
        },
    ),
    (
        "init_rotation_search_global_iterations",
        int,
        {
            "argstr": "-init_rotation.search.global.iterations",
            "help_string": """number of rotations to investigate (Default: 10000)""",
        },
    ),
    # Advanced linear registration stage options Option Group
    (
        "linstage_iterations",
        ty.List[int],
        {
            "argstr": "-linstage.iterations",
            "help_string": """number of iterations for each registration stage, not to be confused with -rigid_niter or -affine_niter. This can be used to generate intermediate diagnostics images (-linstage.diagnostics.prefix) or to change the cost function optimiser during registration (without the need to repeatedly resize the images). (Default: 1 == no repetition)""",
            "sep": ",",
        },
    ),
    (
        "linstage_optimiser_first",
        str,
        {
            "argstr": "-linstage.optimiser.first",
            "help_string": """Cost function optimisation algorithm to use at first iteration of all stages. Valid choices: bbgd (Barzilai-Borwein gradient descent) or gd (simple gradient descent). (Default: bbgd)""",
            "allowed_values": ["bbgd", "bbgd", "gd"],
        },
    ),
    (
        "linstage_optimiser_last",
        str,
        {
            "argstr": "-linstage.optimiser.last",
            "help_string": """Cost function optimisation algorithm to use at last iteration of all stages (if there are more than one). Valid choices: bbgd (Barzilai-Borwein gradient descent) or gd (simple gradient descent). (Default: bbgd)""",
            "allowed_values": ["bbgd", "bbgd", "gd"],
        },
    ),
    (
        "linstage_optimiser_default",
        str,
        {
            "argstr": "-linstage.optimiser.default",
            "help_string": """Cost function optimisation algorithm to use at any stage iteration other than first or last iteration. Valid choices: bbgd (Barzilai-Borwein gradient descent) or gd (simple gradient descent). (Default: bbgd)""",
            "allowed_values": ["bbgd", "bbgd", "gd"],
        },
    ),
    (
        "linstage_diagnostics_prefix",
        str,
        {
            "argstr": "-linstage.diagnostics.prefix",
            "help_string": """generate diagnostics images after every registration stage""",
        },
    ),
    # Non-linear registration options Option Group
    (
        "nl_warp",
        ty.Union[ty.Tuple[Path, Path], bool],
        False,
        {
            "argstr": "-nl_warp",
            "output_file_template": (
                "nl_warp0.mif",
                "nl_warp1.mif",
            ),
            "help_string": """the non-linear warp output defined as two deformation fields, where warp1 can be used to transform image1->image2 and warp2 to transform image2->image1. The deformation fields also encapsulate any linear transformation estimated prior to non-linear registration.""",
        },
    ),
    (
        "nl_warp_full",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-nl_warp_full",
            "output_file_template": "nl_warp_full.mif",
            "help_string": """output all warps used during registration. This saves four different warps that map each image to a midway space and their inverses in a single 5D image file. The 4th image dimension indexes the x,y,z component of the deformation vector and the 5th dimension indexes the field in this order: image1->midway, midway->image1, image2->midway, midway->image2. Where image1->midway defines the field that maps image1 onto the midway space using the reverse convention When linear registration is performed first, the estimated linear transform will be included in the comments of the image header, and therefore the entire linear and non-linear transform can be applied (in either direction) using this output warp file with mrtransform""",
        },
    ),
    (
        "nl_init",
        ImageIn,
        {
            "argstr": "-nl_init",
            "help_string": """initialise the non-linear registration with the supplied warp image. The supplied warp must be in the same format as output using the -nl_warp_full option (i.e. have 4 deformation fields with the linear transforms in the image header)""",
        },
    ),
    (
        "nl_scale",
        ty.List[float],
        {
            "argstr": "-nl_scale",
            "help_string": """use a multi-resolution scheme by defining a scale factor for each level using comma separated values (Default: 0.25,0.5,1.0)""",
            "sep": ",",
        },
    ),
    (
        "nl_niter",
        ty.List[int],
        {
            "argstr": "-nl_niter",
            "help_string": """the maximum number of iterations. This can be specified either as a single number for all multi-resolution levels, or a single value for each level. (Default: 50)""",
            "sep": ",",
        },
    ),
    (
        "nl_update_smooth",
        float,
        {
            "argstr": "-nl_update_smooth",
            "help_string": """regularise the gradient update field with Gaussian smoothing (standard deviation in voxel units, Default 2.0)""",
        },
    ),
    (
        "nl_disp_smooth",
        float,
        {
            "argstr": "-nl_disp_smooth",
            "help_string": """regularise the displacement field with Gaussian smoothing (standard deviation in voxel units, Default 1.0)""",
        },
    ),
    (
        "nl_grad_step",
        float,
        {
            "argstr": "-nl_grad_step",
            "help_string": """the gradient step size for non-linear registration (Default: 0.5)""",
        },
    ),
    (
        "nl_lmax",
        ty.List[int],
        {
            "argstr": "-nl_lmax",
            "help_string": """explicitly set the lmax to be used per scale factor in non-linear FOD registration. By default FOD registration will use lmax 0,2,4 with default scale factors 0.25,0.5,1.0 respectively. Note that no reorientation will be performed with lmax = 0.""",
            "sep": ",",
        },
    ),
    (
        "diagnostics_image",
        ty.Any,
        {
            "argstr": "-diagnostics_image",
            "help_string": """write intermediate images for diagnostics purposes""",
        },
    ),
    # FOD registration options Option Group
    (
        "directions",
        File,
        {
            "argstr": "-directions",
            "help_string": """the directions used for FOD reorientation using apodised point spread functions (Default: 60 directions)""",
        },
    ),
    (
        "noreorientation",
        bool,
        {
            "argstr": "-noreorientation",
            "help_string": """turn off FOD reorientation. Reorientation is on by default if the number of volumes in the 4th dimension corresponds to the number of coefficients in an antipodally symmetric spherical harmonic series (i.e. 6, 15, 28, 45, 66 etc)""",
        },
    ),
    # Multi-contrast options Option Group
    (
        "mc_weights",
        ty.List[float],
        {
            "argstr": "-mc_weights",
            "help_string": """relative weight of images used for multi-contrast registration. Default: 1.0 (equal weighting)""",
            "sep": ",",
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

mrregister_input_spec = specs.SpecInfo(
    name="mrregister_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "transformed",
        specs.MultiInputObj[ImageOut],
        {
            "help_string": """image1 after registration transformed and regridded to the space of image2. Note that -transformed needs to be repeated for each contrast if multi-contrast registration is used.""",
        },
    ),
    (
        "transformed_midway",
        specs.MultiInputObj[ty.Tuple[ImageOut, ImageOut]],
        {
            "help_string": """image1 and image2 after registration transformed and regridded to the midway space. Note that -transformed_midway needs to be repeated for each contrast if multi-contrast registration is used.""",
        },
    ),
    (
        "rigid",
        File,
        {
            "help_string": """the output text file containing the rigid transformation as a 4x4 matrix""",
        },
    ),
    (
        "rigid_1tomidway",
        File,
        {
            "help_string": """the output text file containing the rigid transformation that aligns image1 to image2 in their common midway space as a 4x4 matrix""",
        },
    ),
    (
        "rigid_2tomidway",
        File,
        {
            "help_string": """the output text file containing the rigid transformation that aligns image2 to image1 in their common midway space as a 4x4 matrix""",
        },
    ),
    (
        "rigid_log",
        File,
        {
            "help_string": """write gradient descent parameter evolution to log file""",
        },
    ),
    (
        "affine",
        File,
        {
            "help_string": """the output text file containing the affine transformation as a 4x4 matrix""",
        },
    ),
    (
        "affine_1tomidway",
        File,
        {
            "help_string": """the output text file containing the affine transformation that aligns image1 to image2 in their common midway space as a 4x4 matrix""",
        },
    ),
    (
        "affine_2tomidway",
        File,
        {
            "help_string": """the output text file containing the affine transformation that aligns image2 to image1 in their common midway space as a 4x4 matrix""",
        },
    ),
    (
        "affine_log",
        File,
        {
            "help_string": """write gradient descent parameter evolution to log file""",
        },
    ),
    (
        "nl_warp",
        ty.Tuple[ImageOut, ImageOut],
        {
            "help_string": """the non-linear warp output defined as two deformation fields, where warp1 can be used to transform image1->image2 and warp2 to transform image2->image1. The deformation fields also encapsulate any linear transformation estimated prior to non-linear registration.""",
        },
    ),
    (
        "nl_warp_full",
        ImageOut,
        {
            "help_string": """output all warps used during registration. This saves four different warps that map each image to a midway space and their inverses in a single 5D image file. The 4th image dimension indexes the x,y,z component of the deformation vector and the 5th dimension indexes the field in this order: image1->midway, midway->image1, image2->midway, midway->image2. Where image1->midway defines the field that maps image1 onto the midway space using the reverse convention When linear registration is performed first, the estimated linear transform will be included in the comments of the image header, and therefore the entire linear and non-linear transform can be applied (in either direction) using this output warp file with mrtransform""",
        },
    ),
]
mrregister_output_spec = specs.SpecInfo(
    name="mrregister_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class mrregister(ShellCommandTask):
    """By default this application will perform an affine, followed by non-linear registration.

        FOD registration (with apodised point spread reorientation) will be performed by default if the number of volumes in the 4th dimension equals the number of coefficients in an antipodally symmetric spherical harmonic series (e.g. 6, 15, 28 etc). The -no_reorientation option can be used to force reorientation off if required.

        Non-linear registration computes warps to map from both image1->image2 and image2->image1. Similar to Avants (2008) Med Image Anal. 12(1): 26–41, registration is performed by matching both the image1 and image2 in a 'midway space'. Warps can be saved as two deformation fields that map directly between image1->image2 and image2->image1, or if using -nl_warp_full as a single 5D file that stores all 4 warps image1->mid->image2, and image2->mid->image1. The 5D warp format stores x,y,z deformations in the 4th dimension, and uses the 5th dimension to index the 4 warps. The affine transforms estimated (to midway space) are also stored as comments in the image header. The 5D warp file can be used to reinitialise subsequent registrations, in addition to transforming images to midway space (e.g. for intra-subject alignment in a 2-time-point longitudinal analysis).


        References
        ----------

            * If FOD registration is being performed:
    Raffelt, D.; Tournier, J.-D.; Fripp, J; Crozier, S.; Connelly, A. & Salvado, O. Symmetric diffeomorphic registration of fibre orientation distributions. NeuroImage, 2011, 56(3), 1171-1180

            Raffelt, D.; Tournier, J.-D.; Crozier, S.; Connelly, A. & Salvado, O. Reorientation of fiber orientation distributions using apodized point spread functions. Magnetic Resonance in Medicine, 2012, 67, 844-855

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: David Raffelt (david.raffelt@florey.edu.au) & Max Pietsch (maximilian.pietsch@kcl.ac.uk)

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

    executable = "mrregister"
    input_spec = mrregister_input_spec
    output_spec = mrregister_output_spec
