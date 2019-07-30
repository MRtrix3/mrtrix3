.. _mrview:

mrview
===================

Synopsis
--------

The MRtrix image viewer.

Usage
--------

::

    mrview [ options ] [ image ... ]

-  *image*: An image to be loaded.

Description
-----------

Any images listed as arguments will be loaded and available through the image menu, with the first listed displayed initially. Any subsequent command-line options will be processed as if the corresponding action had been performed through the GUI.

Note that because images loaded as arguments (i.e. simply listed on the command-line) are opened before the GUI is shown, subsequent actions to be performed via the various command-line options must appear after the last argument. This is to avoid confusion about which option will apply to which image. If you need fine control over this, please use the -load or -select_image options. For example:

$ mrview -load image1.mif -interpolation 0 -load image2.mif -interpolation 0

or

$ mrview image1.mif image2.mif -interpolation 0 -select_image 2 -interpolation 0

Options
-------

View options
^^^^^^^^^^^^

-  **-mode index** Switch to view mode specified by the integer index, as per the view menu.

-  **-load image** Load image specified and make it current.

-  **-reset** Reset the view according to current image. This resets the FOV, projection and focus.

-  **-fov value** Set the field of view, in mm.

-  **-focus x,y,z or boolean** Either set the position of the crosshairs in scanner coordinates, with the new position supplied as a comma-separated list of floating-point values or show or hide the focus cross hair using a boolean value as argument.

-  **-target x,y,z** Set the target location for the viewing window (the scanner coordinate that will appear at the centre of the viewing window

-  **-voxel x,y,z** Set the position of the crosshairs in voxel coordinates, relative the image currently displayed. The new position should be supplied as a comma-separated list of floating-point values.

-  **-volume idx** Set the volume index for the image displayed, as a comma-separated list of integers.

-  **-plane index** Set the viewing plane, according to the mappping 0: sagittal; 1: coronal; 2: axial.

-  **-lock yesno** Set whether view is locked to image axes (0: no, 1: yes).

-  **-select_image index** Switch to image number specified, with reference to the list of currently loaded images.

-  **-autoscale** Reset the image scaling to automatically determined range.

-  **-interpolation boolean** Enable or disable image interpolation in main image.

-  **-colourmap index** Switch the image colourmap to that specified, as per the colourmap menu.

-  **-noannotations** Hide all image annotation overlays

-  **-comments boolean** Show or hide image comments overlay.

-  **-voxelinfo boolean** Show or hide voxel information overlay.

-  **-orientationlabel boolean** Show or hide orientation label overlay.

-  **-colourbar boolean** Show or hide colourbar overlay.

-  **-imagevisible boolean** Show or hide the main image.

-  **-intensity_range min,max** Set the image intensity range to that specified.

Window management options
^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-size width,height** Set the size of the view area, in pixel units.

-  **-position x,y** Set the position of the main window, in pixel units.

-  **-fullscreen** Start fullscreen.

-  **-exit** Quit MRView.

Debugging options
^^^^^^^^^^^^^^^^^

-  **-fps** Display frames per second, averaged over the last 10 frames. The maximum over the last 3 seconds is also displayed.

Other options
^^^^^^^^^^^^^

-  **-norealign** do not realign transform to near-default RAS coordinate system (the default behaviour on image load). This is useful to inspect the image and/or header contents as they are actually stored in the header, rather than as MRtrix interprets them.

Overlay tool options
^^^^^^^^^^^^^^^^^^^^

-  **-overlay.load image** Loads the specified image on the overlay tool.

-  **-overlay.opacity value** Sets the overlay opacity to floating value [0-1].

-  **-overlay.colourmap index** Sets the colourmap of the overlay as indexed in the colourmap dropdown menu.

-  **-overlay.colour R,G,B** Specify a manual colour for the overlay, as three comma-separated values

-  **-overlay.intensity Min,Max** Set the intensity windowing of the overlay

-  **-overlay.threshold_min value** Set the lower threshold value of the overlay

-  **-overlay.threshold_max value** Set the upper threshold value of the overlay

-  **-overlay.no_threshold_min** Disable the lower threshold for the overlay

-  **-overlay.no_threshold_max** Disable the upper threshold for the overlay

-  **-overlay.interpolation value** Enable or disable overlay image interpolation.

ROI editor tool options
^^^^^^^^^^^^^^^^^^^^^^^

-  **-roi.load image** Loads the specified image on the ROI editor tool.

-  **-roi.opacity value** Sets the overlay opacity to floating value [0-1].

-  **-roi.colour R,G,B** Sets the colour of the ROI overlay

Tractography tool options
^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-tractography.load tracks** Load the specified tracks file into the tractography tool.

-  **-tractography.thickness value** Line thickness of tractography display, [-1.0, 1.0], default is 0.0.

-  **-tractography.geometry value** The geometry type to use when rendering tractograms (options are: pseudotubes, lines, points)

-  **-tractography.opacity value** Opacity of tractography display, [0.0, 1.0], default is 1.0.

-  **-tractography.slab value** Slab thickness of tractography display, in mm. -1 to turn off crop to slab.

-  **-tractography.lighting value** Toggle the use of lighting of tractogram geometry

-  **-tractography.tsf_load tsf** Load the specified tractography scalar file.

-  **-tractography.tsf_range RangeMin,RangeMax** Set range for the tractography scalar file. Requires -tractography.tsf_load already provided.

-  **-tractography.tsf_thresh ThresholdMin,ThesholdMax** Set thresholds for the tractography scalar file. Requires -tractography.tsf_load already provided.

ODF tool options
^^^^^^^^^^^^^^^^

-  **-odf.load_sh image** Loads the specified SH-based ODF image on the ODF tool.

-  **-odf.load_tensor image** Loads the specified tensor image on the ODF tool.

-  **-odf.load_dixel image** Loads the specified dixel-based image on the ODF tool.

Fixel plot tool options
^^^^^^^^^^^^^^^^^^^^^^^

-  **-fixel.load image** Load a fixel file (any file inside a fixel directory, or an old .msf / .msh legacy format file) into the fixel tool.

Connectome tool options
^^^^^^^^^^^^^^^^^^^^^^^

-  **-connectome.init image** Initialise the connectome tool using a parcellation image.

-  **-connectome.load path** Load a matrix file into the connectome tool.

Screen Capture tool options
^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-capture.folder path** Set the output folder for the screen capture tool.

-  **-capture.prefix string** Set the output file prefix for the screen capture tool.

-  **-capture.grab** Start the screen capture process.

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

Tournier, J.-D.; Calamante, F. & Connelly, A. MRtrix: Diffusion tractography in crossing fiber regions. Int. J. Imaging Syst. Technol., 2012, 22, 53-66

--------------



**Author:** J-Donald Tournier (jdtournier@gmail.com), Dave Raffelt (david.raffelt@florey.edu.au), Robert E. Smith (robert.smith@florey.edu.au), Rami Tabbara (rami.tabbara@florey.edu.au), Max Pietsch (maximilian.pietsch@kcl.ac.uk), Thijs Dhollander (thijs.dhollander@gmail.com)

**Copyright:** Copyright (c) 2008-2018 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/

MRtrix3 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/


