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
            "help_string": """The image containing the source data. The type of image data required depends on the algorithm used (see Description section).""",
            "mandatory": True,
        },
    ),
    (
        "tracks",
        Tracks,
        {
            "argstr": "",
            "position": 1,
            "help_string": """the output file containing the tracks generated.""",
            "mandatory": True,
        },
    ),
    (
        "algorithm",
        str,
        {
            "argstr": "-algorithm",
            "help_string": """specify the tractography algorithm to use. Valid choices are: FACT, iFOD1, iFOD2, Nulldist1, Nulldist2, SD_Stream, Seedtest, Tensor_Det, Tensor_Prob (default: iFOD2).""",
            "allowed_values": [
                "fact",
                "fact",
                "ifod1",
                "ifod2",
                "nulldist1",
                "nulldist2",
                "sd_stream",
                "seedtest",
                "tensor_det",
                "tensor_prob",
            ],
        },
    ),
    # Streamlines tractography options Option Group
    (
        "select",
        int,
        {
            "argstr": "-select",
            "help_string": """set the desired number of streamlines to be selected by tckgen, after all selection criteria have been applied (i.e. inclusion/exclusion ROIs, min/max length, etc). tckgen will keep seeding streamlines until this number of streamlines have been selected, or the maximum allowed number of seeds has been exceeded (see -seeds option). By default, 5000 streamlines are to be selected. Set to zero to disable, which will result in streamlines being seeded until the number specified by -seeds has been reached.""",
        },
    ),
    (
        "step",
        float,
        {
            "argstr": "-step",
            "help_string": """set the step size of the algorithm in mm (defaults: for first-order algorithms, 0.1 x voxelsize; if using RK4, 0.25 x voxelsize; for iFOD2: 0.5 x voxelsize).""",
        },
    ),
    (
        "angle",
        float,
        {
            "argstr": "-angle",
            "help_string": """set the maximum angle in degrees between successive steps (defaults: 60 for deterministic algorithms; 15 for iFOD1 / nulldist1; 45 for iFOD2 / nulldist2)""",
        },
    ),
    (
        "minlength",
        float,
        {
            "argstr": "-minlength",
            "help_string": """set the minimum length of any track in mm (defaults: without ACT, 5 x voxelsize; with ACT, 2 x voxelsize).""",
        },
    ),
    (
        "maxlength",
        float,
        {
            "argstr": "-maxlength",
            "help_string": """set the maximum length of any track in mm (default: 100 x voxelsize).""",
        },
    ),
    (
        "cutoff",
        float,
        {
            "argstr": "-cutoff",
            "help_string": """set the FOD amplitude / fixel size / tensor FA cutoff for terminating tracks (defaults: 0.1 for FOD-based algorithms; 0.1 for fixel-based algorithms; 0.1 for tensor-based algorithms; threshold multiplied by 0.5 when using ACT).""",
        },
    ),
    (
        "trials",
        int,
        {
            "argstr": "-trials",
            "help_string": """set the maximum number of sampling trials at each point (only used for iFOD1 / iFOD2) (default: 1000).""",
        },
    ),
    (
        "noprecomputed",
        bool,
        {
            "argstr": "-noprecomputed",
            "help_string": """do NOT pre-compute legendre polynomial values. Warning: this will slow down the algorithm by a factor of approximately 4.""",
        },
    ),
    (
        "rk4",
        bool,
        {
            "argstr": "-rk4",
            "help_string": """use 4th-order Runge-Kutta integration (slower, but eliminates curvature overshoot in 1st-order deterministic methods)""",
        },
    ),
    (
        "stop",
        bool,
        {
            "argstr": "-stop",
            "help_string": """stop propagating a streamline once it has traversed all include regions""",
        },
    ),
    (
        "downsample",
        int,
        {
            "argstr": "-downsample",
            "help_string": """downsample the generated streamlines to reduce output file size (default is (samples-1) for iFOD2, no downsampling for all other algorithms)""",
        },
    ),
    # Tractography seeding mechanisms; at least one must be provided Option Group
    (
        "seed_image",
        specs.MultiInputObj[ImageIn],
        {
            "argstr": "-seed_image",
            "help_string": """seed streamlines entirely at random within a mask image """,
        },
    ),
    (
        "seed_sphere",
        specs.MultiInputObj[ty.List[float]],
        {
            "argstr": "-seed_sphere",
            "help_string": """spherical seed as four comma-separated values (XYZ position and radius)""",
            "sep": ",",
        },
    ),
    (
        "seed_random_per_voxel",
        specs.MultiInputObj[ty.Tuple[ImageIn, ImageIn]],
        {
            "argstr": "-seed_random_per_voxel",
            "help_string": """seed a fixed number of streamlines per voxel in a mask image; random placement of seeds in each voxel""",
        },
    ),
    (
        "seed_grid_per_voxel",
        specs.MultiInputObj[ty.Tuple[ImageIn, ImageIn]],
        {
            "argstr": "-seed_grid_per_voxel",
            "help_string": """seed a fixed number of streamlines per voxel in a mask image; place seeds on a 3D mesh grid (grid_size argument is per axis; so a grid_size of 3 results in 27 seeds per voxel)""",
        },
    ),
    (
        "seed_rejection",
        specs.MultiInputObj[ImageIn],
        {
            "argstr": "-seed_rejection",
            "help_string": """seed from an image using rejection sampling (higher values = more probable to seed from)""",
        },
    ),
    (
        "seed_gmwmi",
        specs.MultiInputObj[ImageIn],
        {
            "argstr": "-seed_gmwmi",
            "help_string": """seed from the grey matter - white matter interface (only valid if using ACT framework). Input image should be a 3D seeding volume; seeds drawn within this image will be optimised to the interface using the 5TT image provided using the -act option.""",
        },
    ),
    (
        "seed_dynamic",
        ImageIn,
        {
            "argstr": "-seed_dynamic",
            "help_string": """determine seed points dynamically using the SIFT model (must not provide any other seeding mechanism). Note that while this seeding mechanism improves the distribution of reconstructed streamlines density, it should NOT be used as a substitute for the SIFT method itself.""",
        },
    ),
    # Tractography seeding options and parameters Option Group
    (
        "seeds",
        int,
        {
            "argstr": "-seeds",
            "help_string": """set the number of seeds that tckgen will attempt to track from. If this option is NOT provided, the default number of seeds is set to 1000× the number of selected streamlines. If -select is NOT also specified, tckgen will continue tracking until this number of seeds has been attempted. However, if -select is also specified, tckgen will stop when the number of seeds attempted reaches the number specified here, OR when the number of streamlines selected reaches the number requested with the -select option. This can be used to prevent the program from running indefinitely when no or very few streamlines can be found that match the selection criteria. Setting this to zero will cause tckgen to keep attempting seeds until the number specified by -select has been reached.""",
        },
    ),
    (
        "max_attempts_per_seed",
        int,
        {
            "argstr": "-max_attempts_per_seed",
            "help_string": """set the maximum number of times that the tracking algorithm should attempt to find an appropriate tracking direction from a given seed point. This should be set high enough to ensure that an actual plausible seed point is not discarded prematurely as being unable to initiate tracking from. Higher settings may affect performance if many seeds are genuinely impossible to track from, as many attempts will still be made in vain for such seeds. (default: 1000)""",
        },
    ),
    (
        "seed_cutoff",
        float,
        {
            "argstr": "-seed_cutoff",
            "help_string": """set the minimum FA or FOD amplitude for seeding tracks (default is the same as the normal -cutoff).""",
        },
    ),
    (
        "seed_unidirectional",
        bool,
        {
            "argstr": "-seed_unidirectional",
            "help_string": """track from the seed point in one direction only (default is to track in both directions).""",
        },
    ),
    (
        "seed_direction",
        ty.List[float],
        {
            "argstr": "-seed_direction",
            "help_string": """specify a seeding direction for the tracking (this should be supplied as a vector of 3 comma-separated values.""",
            "sep": ",",
        },
    ),
    (
        "output_seeds",
        ty.Union[Path, bool],
        False,
        {
            "argstr": "-output_seeds",
            "output_file_template": "output_seeds.txt",
            "help_string": """output the seed location of all successful streamlines to a file""",
        },
    ),
    # Region Of Interest processing options Option Group
    (
        "include",
        specs.MultiInputObj[ty.Any],
        {
            "argstr": "-include",
            "help_string": """specify an inclusion region of interest, as either a binary mask image, or as a sphere using 4 comma-separared values (x,y,z,radius). Streamlines must traverse ALL inclusion regions to be accepted.""",
        },
    ),
    (
        "include_ordered",
        specs.MultiInputObj[str],
        {
            "argstr": "-include_ordered",
            "help_string": """specify an inclusion region of interest, as either a binary mask image, or as a sphere using 4 comma-separared values (x,y,z,radius). Streamlines must traverse ALL inclusion_ordered regions in the order they are specified in order to be accepted.""",
        },
    ),
    (
        "exclude",
        specs.MultiInputObj[ty.Any],
        {
            "argstr": "-exclude",
            "help_string": """specify an exclusion region of interest, as either a binary mask image, or as a sphere using 4 comma-separared values (x,y,z,radius). Streamlines that enter ANY exclude region will be discarded.""",
        },
    ),
    (
        "mask",
        specs.MultiInputObj[ty.Any],
        {
            "argstr": "-mask",
            "help_string": """specify a masking region of interest, as either a binary mask image, or as a sphere using 4 comma-separared values (x,y,z,radius). If defined, streamlines exiting the mask will be truncated.""",
        },
    ),
    # Anatomically-Constrained Tractography options Option Group
    (
        "act",
        ImageIn,
        {
            "argstr": "-act",
            "help_string": """use the Anatomically-Constrained Tractography framework during tracking; provided image must be in the 5TT (five-tissue-type) format""",
        },
    ),
    (
        "backtrack",
        bool,
        {
            "argstr": "-backtrack",
            "help_string": """allow tracks to be truncated and re-tracked if a poor structural termination is encountered""",
        },
    ),
    (
        "crop_at_gmwmi",
        bool,
        {
            "argstr": "-crop_at_gmwmi",
            "help_string": """crop streamline endpoints more precisely as they cross the GM-WM interface""",
        },
    ),
    # Options specific to the iFOD tracking algorithms Option Group
    (
        "power",
        float,
        {
            "argstr": "-power",
            "help_string": """raise the FOD to the power specified (defaults are: 1.0 for iFOD1; 1.0/nsamples for iFOD2).""",
        },
    ),
    # Options specific to the iFOD2 tracking algorithm Option Group
    (
        "samples",
        int,
        {
            "argstr": "-samples",
            "help_string": """set the number of FOD samples to take per step (Default: 4).""",
        },
    ),
    # DW gradient table import options Option Group
    (
        "grad",
        File,
        {
            "argstr": "-grad",
            "help_string": """Provide the diffusion-weighted gradient scheme used in the acquisition in a text file. This should be supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.""",
        },
    ),
    (
        "fslgrad",
        ty.Tuple[File, File],
        {
            "argstr": "-fslgrad",
            "help_string": """Provide the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format files. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.""",
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

tckgen_input_spec = specs.SpecInfo(
    name="tckgen_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = [
    (
        "output_seeds",
        File,
        {
            "help_string": """output the seed location of all successful streamlines to a file""",
        },
    ),
]
tckgen_output_spec = specs.SpecInfo(
    name="tckgen_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class tckgen(ShellCommandTask):
    """By default, tckgen produces a fixed number of streamlines, by attempting to seed from new random locations until the target number of streamlines have been selected (in other words, after all inclusion & exclusion criteria have been applied), or the maximum number of seeds has been exceeded (by default, this is 1000 x the desired number of selected streamlines). Use the -select and/or -seeds options to modify as required. See also the Seeding options section for alternative seeding strategies.

        Below is a list of available tracking algorithms, the input image data that they require, and a brief description of their behaviour:

        - FACT: Fiber Assigned by Continuous Tracking. A deterministic algorithm that takes as input a 4D image, with 3xN volumes, where N is the maximum number of fiber orientations in a voxel. Each triplet of volumes represents a 3D vector corresponding to a fiber orientation; the length of the vector additionally indicates some measure of density or anisotropy. As streamlines move from one voxel to another, the fiber orientation most collinear with the streamline orientation is selected (i.e. there is no intra-voxel interpolation).

        - iFOD1: First-order Integration over Fiber Orientation Distributions. A probabilistic algorithm that takes as input a Fiber Orientation Distribution (FOD) image represented in the Spherical Harmonic (SH) basis. At each streamline step, random samples from the local (trilinear interpolated) FOD are taken. A streamline is more probable to follow orientations where the FOD amplitude is large; but it may also rarely traverse orientations with small FOD amplitude.

        - iFOD2 (default): Second-order Integration over Fiber Orientation Distributions. A probabilistic algorithm that takes as input a Fiber Orientation Distribution (FOD) image represented in the Spherical Harmonic (SH) basis. Candidate streamline paths (based on short curved "arcs") are drawn, and the underlying (trilinear-interpolated) FOD amplitudes along those arcs are sampled. A streamline is more probable to follow a path where the FOD amplitudes along that path are large; but it may also rarely traverse orientations where the FOD amplitudes are small, as long as the amplitude remains above the FOD amplitude threshold along the entire path.

        - NullDist1 / NullDist2: Null Distribution tracking algorithms. These probabilistic algorithms expect as input the same image that was used when invoking the corresponding algorithm for which the null distribution is sought. These algorithms generate streamlines based on random orientation samples; that is, no image information relating to fiber orientations is used, and streamlines trajectories are determined entirely from random sampling. The NullDist2 algorithm is designed to be used in conjunction with iFOD2; NullDist1 should be used in conjunction with any first-order algorithm.

        - SD_STREAM: Streamlines tractography based on Spherical Deconvolution (SD). A deterministic algorithm that takes as input a Fiber Orientation Distribution (FOD) image represented in the Spherical Harmonic (SH) basis. At each streamline step, the local (trilinear-interpolated) FOD is sampled, and from the current streamline tangent orientation, a Newton optimisation on the sphere is performed in order to locate the orientation of the nearest FOD amplitude peak.

        - SeedTest: A dummy streamlines algorithm used for testing streamline seeding mechanisms. Any image can be used as input; the image will not be used in any way. For each seed point generated by the seeding mechanism(s), a streamline containing a single point corresponding to that seed location will be written to the output track file.

        - Tensor_Det: A deterministic algorithm that takes as input a 4D diffusion-weighted image (DWI) series. At each streamline step, the diffusion tensor is fitted to the local (trilinear-interpolated) diffusion data, and the streamline trajectory is determined as the principal eigenvector of that tensor.

        - Tensor_Prob: A probabilistic algorithm that takes as input a 4D diffusion-weighted image (DWI) series. Within each image voxel, a residual bootstrap is performed to obtain a unique realisation of the DWI data in that voxel for each streamline. These data are then sampled via trilinear interpolation at each streamline step, the diffusion tensor model is fitted, and the streamline follows the orientation of the principal eigenvector of that tensor.

        Note that the behaviour of the -angle option varies slightly depending on the order of integration: for any first-order method, this angle corresponds to the deviation in streamline trajectory per step; for higher-order methods, this corresponds to the change in underlying fibre orientation between the start and end points of each step.


        References
        ----------

            References based on streamlines algorithm used:

            * FACT:
    Mori, S.; Crain, B. J.; Chacko, V. P. & van Zijl, P. C. M. Three-dimensional tracking of axonal projections in the brain by magnetic resonance imaging. Annals of Neurology, 1999, 45, 265-269

            * iFOD1 or SD_STREAM:
    Tournier, J.-D.; Calamante, F. & Connelly, A. MRtrix: Diffusion tractography in crossing fiber regions. Int. J. Imaging Syst. Technol., 2012, 22, 53-66

            * iFOD2:
    Tournier, J.-D.; Calamante, F. & Connelly, A. Improved probabilistic streamlines tractography by 2nd order integration over fibre orientation distributions. Proceedings of the International Society for Magnetic Resonance in Medicine, 2010, 1670

            * Nulldist1 / Nulldist2:
    Morris, D. M.; Embleton, K. V. & Parker, G. J. Probabilistic fibre tracking: Differentiation of connections from chance events. NeuroImage, 2008, 42, 1329-1339

            * Tensor_Det:
    Basser, P. J.; Pajevic, S.; Pierpaoli, C.; Duda, J. & Aldroubi, A. In vivo fiber tractography using DT-MRI data. Magnetic Resonance in Medicine, 2000, 44, 625-632

            * Tensor_Prob:
    Jones, D. Tractography Gone Wild: Probabilistic Fibre Tracking Using the Wild Bootstrap With Diffusion Tensor MRI. IEEE Transactions on Medical Imaging, 2008, 27, 1268-1274

            References based on command-line options:

            * -rk4:
    Basser, P. J.; Pajevic, S.; Pierpaoli, C.; Duda, J. & Aldroubi, A. In vivo fiber tractography using DT-MRI data. Magnetic Resonance in Medicine, 2000, 44, 625-632

            * -act, -backtrack, -seed_gmwmi:
    Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. Anatomically-constrained tractography: Improved diffusion MRI streamlines tractography through effective use of anatomical information. NeuroImage, 2012, 62, 1924-1938

            * -seed_dynamic:
    Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. SIFT2: Enabling dense quantitative assessment of brain white matter connectivity using streamlines tractography. NeuroImage, 2015, 119, 338-351

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: J-Donald Tournier (jdtournier@gmail.com) and Robert E. Smith (robert.smith@florey.edu.au)

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

    executable = "tckgen"
    input_spec = tckgen_input_spec
    output_spec = tckgen_output_spec
