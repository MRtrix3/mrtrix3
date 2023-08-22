# Auto-generated from MRtrix C++ command with '__print_usage_pydra__' secret option

import typing as ty
from pathlib import Path  # noqa: F401
from fileformats.generic import File, Directory  # noqa: F401
from fileformats.medimage_mrtrix3 import ImageIn, ImageOut, Tracks  # noqa: F401
from pydra.engine import specs, ShellCommandTask


input_fields = [
    # Arguments
    (
        "image_",
        specs.MultiInputObj[ImageIn],
        {
            "argstr": "",
            "position": 0,
            "help_string": """An image to be loaded.""",
        },
    ),
    # View options Option Group
    (
        "mode",
        specs.MultiInputObj[int],
        {
            "argstr": "-mode",
            "help_string": """Switch to view mode specified by the integer index, as per the view menu.""",
        },
    ),
    (
        "load",
        specs.MultiInputObj[ImageIn],
        {
            "argstr": "-load",
            "help_string": """Load image specified and make it current.""",
        },
    ),
    (
        "reset",
        specs.MultiInputObj[bool],
        {
            "argstr": "-reset",
            "help_string": """Reset the view according to current image. This resets the FOV, projection and focus.""",
        },
    ),
    (
        "fov",
        specs.MultiInputObj[float],
        {
            "argstr": "-fov",
            "help_string": """Set the field of view, in mm.""",
        },
    ),
    (
        "focus",
        specs.MultiInputObj[ty.Any],
        {
            "argstr": "-focus",
            "help_string": """Either set the position of the crosshairs in scanner coordinates, with the new position supplied as a comma-separated list of floating-point values or show or hide the focus cross hair using a boolean value as argument.""",
        },
    ),
    (
        "target",
        specs.MultiInputObj[ty.List[float]],
        {
            "argstr": "-target",
            "help_string": """Set the target location for the viewing window (the scanner coordinate that will appear at the centre of the viewing window""",
            "sep": ",",
        },
    ),
    (
        "orientation",
        ty.List[float],
        {
            "argstr": "-orientation",
            "help_string": """Set the orientation of the camera for the viewing window, in the form of a quaternion representing the rotation away from the z-axis. This should be provided as a list of 4 comma-separated floating point values (this will be automatically normalised).""",
            "sep": ",",
        },
    ),
    (
        "voxel",
        specs.MultiInputObj[ty.List[float]],
        {
            "argstr": "-voxel",
            "help_string": """Set the position of the crosshairs in voxel coordinates, relative the image currently displayed. The new position should be supplied as a comma-separated list of floating-point values.""",
            "sep": ",",
        },
    ),
    (
        "volume",
        specs.MultiInputObj[ty.List[int]],
        {
            "argstr": "-volume",
            "help_string": """Set the volume index for the image displayed, as a comma-separated list of integers.""",
            "sep": ",",
        },
    ),
    (
        "plane",
        specs.MultiInputObj[int],
        {
            "argstr": "-plane",
            "help_string": """Set the viewing plane, according to the mappping 0: sagittal; 1: coronal; 2: axial.""",
        },
    ),
    (
        "lock",
        specs.MultiInputObj[bool],
        {
            "argstr": "-lock",
            "help_string": """Set whether view is locked to image axes (0: no, 1: yes).""",
        },
    ),
    (
        "select_image",
        specs.MultiInputObj[int],
        {
            "argstr": "-select_image",
            "help_string": """Switch to image number specified, with reference to the list of currently loaded images.""",
        },
    ),
    (
        "autoscale",
        specs.MultiInputObj[bool],
        {
            "argstr": "-autoscale",
            "help_string": """Reset the image scaling to automatically determined range.""",
        },
    ),
    (
        "interpolation",
        specs.MultiInputObj[bool],
        {
            "argstr": "-interpolation",
            "help_string": """Enable or disable image interpolation in main image.""",
        },
    ),
    (
        "colourmap",
        specs.MultiInputObj[int],
        {
            "argstr": "-colourmap",
            "help_string": """Switch the image colourmap to that specified, as per the colourmap menu.""",
        },
    ),
    (
        "noannotations",
        specs.MultiInputObj[bool],
        {
            "argstr": "-noannotations",
            "help_string": """Hide all image annotation overlays""",
        },
    ),
    (
        "comments",
        specs.MultiInputObj[bool],
        {
            "argstr": "-comments",
            "help_string": """Show or hide image comments overlay.""",
        },
    ),
    (
        "voxelinfo",
        specs.MultiInputObj[bool],
        {
            "argstr": "-voxelinfo",
            "help_string": """Show or hide voxel information overlay.""",
        },
    ),
    (
        "orientlabel",
        specs.MultiInputObj[bool],
        {
            "argstr": "-orientlabel",
            "help_string": """Show or hide orientation label overlay.""",
        },
    ),
    (
        "colourbar",
        specs.MultiInputObj[bool],
        {
            "argstr": "-colourbar",
            "help_string": """Show or hide colourbar overlay.""",
        },
    ),
    (
        "imagevisible",
        specs.MultiInputObj[bool],
        {
            "argstr": "-imagevisible",
            "help_string": """Show or hide the main image.""",
        },
    ),
    (
        "intensity_range",
        specs.MultiInputObj[ty.List[float]],
        {
            "argstr": "-intensity_range",
            "help_string": """Set the image intensity range to that specified.""",
            "sep": ",",
        },
    ),
    # Window management options Option Group
    (
        "size",
        specs.MultiInputObj[ty.List[int]],
        {
            "argstr": "-size",
            "help_string": """Set the size of the view area, in pixel units.""",
            "sep": ",",
        },
    ),
    (
        "position",
        specs.MultiInputObj[ty.List[int]],
        {
            "argstr": "-position",
            "help_string": """Set the position of the main window, in pixel units.""",
            "sep": ",",
        },
    ),
    (
        "fullscreen",
        bool,
        {
            "argstr": "-fullscreen",
            "help_string": """Start fullscreen.""",
        },
    ),
    (
        "exit",
        bool,
        {
            "argstr": "-exit",
            "help_string": """Quit MRView.""",
        },
    ),
    # Sync Options Option Group
    (
        "sync_focus",
        bool,
        {
            "argstr": "-sync.focus",
            "help_string": """Sync the focus with other MRView windows that also have this turned on.""",
        },
    ),
    # Debugging options Option Group
    (
        "fps",
        bool,
        {
            "argstr": "-fps",
            "help_string": """Display frames per second, averaged over the last 10 frames. The maximum over the last 3 seconds is also displayed.""",
        },
    ),
    # Overlay tool options Option Group
    (
        "overlay_load",
        specs.MultiInputObj[ImageIn],
        {
            "argstr": "-overlay.load",
            "help_string": """Loads the specified image on the overlay tool.""",
        },
    ),
    (
        "overlay_opacity",
        specs.MultiInputObj[float],
        {
            "argstr": "-overlay.opacity",
            "help_string": """Sets the overlay opacity to floating value [0-1].""",
        },
    ),
    (
        "overlay_colourmap",
        specs.MultiInputObj[int],
        {
            "argstr": "-overlay.colourmap",
            "help_string": """Sets the colourmap of the overlay as indexed in the colourmap dropdown menu.""",
        },
    ),
    (
        "overlay_colour",
        specs.MultiInputObj[ty.List[float]],
        {
            "argstr": "-overlay.colour",
            "help_string": """Specify a manual colour for the overlay, as three comma-separated values""",
            "sep": ",",
        },
    ),
    (
        "overlay_intensity",
        specs.MultiInputObj[ty.List[float]],
        {
            "argstr": "-overlay.intensity",
            "help_string": """Set the intensity windowing of the overlay""",
            "sep": ",",
        },
    ),
    (
        "overlay_threshold_min",
        specs.MultiInputObj[float],
        {
            "argstr": "-overlay.threshold_min",
            "help_string": """Set the lower threshold value of the overlay""",
        },
    ),
    (
        "overlay_threshold_max",
        specs.MultiInputObj[float],
        {
            "argstr": "-overlay.threshold_max",
            "help_string": """Set the upper threshold value of the overlay""",
        },
    ),
    (
        "overlay_no_threshold_min",
        specs.MultiInputObj[bool],
        {
            "argstr": "-overlay.no_threshold_min",
            "help_string": """Disable the lower threshold for the overlay""",
        },
    ),
    (
        "overlay_no_threshold_max",
        specs.MultiInputObj[bool],
        {
            "argstr": "-overlay.no_threshold_max",
            "help_string": """Disable the upper threshold for the overlay""",
        },
    ),
    (
        "overlay_interpolation",
        specs.MultiInputObj[bool],
        {
            "argstr": "-overlay.interpolation",
            "help_string": """Enable or disable overlay image interpolation.""",
        },
    ),
    # ROI editor tool options Option Group
    (
        "roi_load",
        specs.MultiInputObj[ImageIn],
        {
            "argstr": "-roi.load",
            "help_string": """Loads the specified image on the ROI editor tool.""",
        },
    ),
    (
        "roi_opacity",
        specs.MultiInputObj[float],
        {
            "argstr": "-roi.opacity",
            "help_string": """Sets the overlay opacity to floating value [0-1].""",
        },
    ),
    (
        "roi_colour",
        specs.MultiInputObj[ty.List[float]],
        {
            "argstr": "-roi.colour",
            "help_string": """Sets the colour of the ROI overlay""",
            "sep": ",",
        },
    ),
    # Tractography tool options Option Group
    (
        "tractography_load",
        specs.MultiInputObj[File],
        {
            "argstr": "-tractography.load",
            "help_string": """Load the specified tracks file into the tractography tool.""",
        },
    ),
    (
        "tractography_thickness",
        specs.MultiInputObj[float],
        {
            "argstr": "-tractography.thickness",
            "help_string": """Line thickness of tractography display, [-1.0, 1.0], default is 0.0.""",
        },
    ),
    (
        "tractography_geometry",
        specs.MultiInputObj[str],
        {
            "argstr": "-tractography.geometry",
            "help_string": """The geometry type to use when rendering tractograms (options are: pseudotubes, lines, points)""",
            "allowed_values": ["pseudotubes", "pseudotubes", "lines", "points"],
        },
    ),
    (
        "tractography_opacity",
        specs.MultiInputObj[float],
        {
            "argstr": "-tractography.opacity",
            "help_string": """Opacity of tractography display, [0.0, 1.0], default is 1.0.""",
        },
    ),
    (
        "tractography_slab",
        specs.MultiInputObj[float],
        {
            "argstr": "-tractography.slab",
            "help_string": """Slab thickness of tractography display, in mm. -1 to turn off crop to slab.""",
        },
    ),
    (
        "tractography_lighting",
        specs.MultiInputObj[bool],
        {
            "argstr": "-tractography.lighting",
            "help_string": """Toggle the use of lighting of tractogram geometry""",
        },
    ),
    (
        "tractography_colour",
        specs.MultiInputObj[ty.List[float]],
        {
            "argstr": "-tractography.colour",
            "help_string": """Specify a manual colour for the tractogram, as three comma-separated values""",
            "sep": ",",
        },
    ),
    (
        "tractography_tsf_load",
        specs.MultiInputObj[File],
        {
            "argstr": "-tractography.tsf_load",
            "help_string": """Load the specified tractography scalar file.""",
        },
    ),
    (
        "tractography_tsf_range",
        specs.MultiInputObj[ty.List[float]],
        {
            "argstr": "-tractography.tsf_range",
            "help_string": """Set range for the tractography scalar file. Requires -tractography.tsf_load already provided.""",
            "sep": ",",
        },
    ),
    (
        "tractography_tsf_thresh",
        specs.MultiInputObj[ty.List[float]],
        {
            "argstr": "-tractography.tsf_thresh",
            "help_string": """Set thresholds for the tractography scalar file. Requires -tractography.tsf_load already provided.""",
            "sep": ",",
        },
    ),
    (
        "tractography_tsf_colourmap",
        specs.MultiInputObj[int],
        {
            "argstr": "-tractography.tsf_colourmap",
            "help_string": """Sets the colourmap of the .tsf file as indexed in the tsf colourmap dropdown menu. Requires -tractography.tsf_load already.""",
        },
    ),
    # ODF tool options Option Group
    (
        "odf_load_sh",
        specs.MultiInputObj[ImageIn],
        {
            "argstr": "-odf.load_sh",
            "help_string": """Loads the specified SH-based ODF image on the ODF tool.""",
        },
    ),
    (
        "odf_load_tensor",
        specs.MultiInputObj[ImageIn],
        {
            "argstr": "-odf.load_tensor",
            "help_string": """Loads the specified tensor image on the ODF tool.""",
        },
    ),
    (
        "odf_load_dixel",
        specs.MultiInputObj[ImageIn],
        {
            "argstr": "-odf.load_dixel",
            "help_string": """Loads the specified dixel-based image on the ODF tool.""",
        },
    ),
    # Fixel plot tool options Option Group
    (
        "fixel_load",
        specs.MultiInputObj[ImageIn],
        {
            "argstr": "-fixel.load",
            "help_string": """Load a fixel file (any file inside a fixel directory, or an old .msf / .msh legacy format file) into the fixel tool.""",
        },
    ),
    # Connectome tool options Option Group
    (
        "connectome_init",
        ImageIn,
        {
            "argstr": "-connectome.init",
            "help_string": """Initialise the connectome tool using a parcellation image.""",
        },
    ),
    (
        "connectome_load",
        specs.MultiInputObj[File],
        {
            "argstr": "-connectome.load",
            "help_string": """Load a matrix file into the connectome tool.""",
        },
    ),
    # Screen Capture tool options Option Group
    (
        "capture_folder",
        specs.MultiInputObj[str],
        {
            "argstr": "-capture.folder",
            "help_string": """Set the output folder for the screen capture tool.""",
        },
    ),
    (
        "capture_prefix",
        specs.MultiInputObj[str],
        {
            "argstr": "-capture.prefix",
            "help_string": """Set the output file prefix for the screen capture tool.""",
        },
    ),
    (
        "capture_grab",
        specs.MultiInputObj[bool],
        {
            "argstr": "-capture.grab",
            "help_string": """Start the screen capture process.""",
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

mrview_input_spec = specs.SpecInfo(
    name="mrview_input", fields=input_fields, bases=(specs.ShellSpec,)
)


output_fields = []
mrview_output_spec = specs.SpecInfo(
    name="mrview_output", fields=output_fields, bases=(specs.ShellOutSpec,)
)


class mrview(ShellCommandTask):
    """Any images listed as arguments will be loaded and available through the image menu, with the first listed displayed initially. Any subsequent command-line options will be processed as if the corresponding action had been performed through the GUI.

        Note that because images loaded as arguments (i.e. simply listed on the command-line) are opened before the GUI is shown, subsequent actions to be performed via the various command-line options must appear after the last argument. This is to avoid confusion about which option will apply to which image. If you need fine control over this, please use the -load or -select_image options. For example:

        $ mrview -load image1.mif -interpolation 0 -load image2.mif -interpolation 0

        or

        $ mrview image1.mif image2.mif -interpolation 0 -select_image 2 -interpolation 0


        References
        ----------

            Tournier, J.-D.; Calamante, F. & Connelly, A. MRtrix: Diffusion tractography in crossing fiber regions. Int. J. Imaging Syst. Technol., 2012, 22, 53-66

            Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137


        MRtrix
        ------

            Version:3.0.4-691-g05465c3c-dirty, built Sep 29 2023

            Author: J-Donald Tournier (jdtournier@gmail.com), Dave Raffelt (david.raffelt@florey.edu.au), Robert E. Smith (robert.smith@florey.edu.au), Rami Tabbara (rami.tabbara@florey.edu.au), Max Pietsch (maximilian.pietsch@kcl.ac.uk), Thijs Dhollander (thijs.dhollander@gmail.com)

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

    executable = "mrview"
    input_spec = mrview_input_spec
    output_spec = mrview_output_spec
