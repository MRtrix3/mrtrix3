.. _mrview:

mrview
===========

Synopsis
--------

::

    mrview [ options ] [ image ... ]

-  *image*: an image to be loaded.

Description
-----------

the MRtrix image viewer.

Options
-------

View options
^^^^^^^^^^^^

-  **-mode index** Switch to view mode specified by the integer index. as per the view menu.

-  **-load image** Load image specified and make it current.

-  **-reset** Reset the view according to current image. This resets the FOV, projection, and focus.

-  **-fov value** Set the field of view, in mm.

-  **-focus x,y,z** Set the position of the crosshairs in scanner coordinates, with the new position supplied as a comma-separated list of floating-point values.

-  **-voxel x,y,z** Set the position of the crosshairs in voxel coordinates, relative the image currently displayed. The new position should be supplied as a comma-separated list of floating-point values.

-  **-plane index** Set the viewing plane, according to the mappping 0: sagittal; 1: coronal; 2: axial.

-  **-lock yesno** Set whether view is locked to image axes (0: no, 1: yes).

-  **-select_image index** Switch to image number specified, with reference to the list of currently loaded images.

-  **-autoscale** Reset the image scaling to automatically determined range.

-  **-interpolation_on** Enable the image interpolation.

-  **-interpolation_off** Disable the image interpolation.

-  **-colourmap index** Switch the image colourmap to that specified, as per the colourmap menu.

-  **-intensity_range min,max** Set the image intensity range to that specified

Window management options
^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-size width,height** Set the size of the view area, in pixel units.

-  **-position x,y** Set the position of the main window, in pixel units.

-  **-fullscreen** Start fullscreen.

-  **-nointerpolation** Disable interpolation of the image.

-  **-exit** quit MRView

Debugging options
^^^^^^^^^^^^^^^^^

-  **-fps** Display frames per second, averaged over the last 10 frames. The maximum over the last 3 seconds is also displayed.

Overlay tool options
^^^^^^^^^^^^^^^^^^^^

-  **-overlay.load image** Loads the specified image on the overlay tool.

-  **-overlay.opacity value** Sets the overlay opacity to floating value [0-1].

-  **-overlay.interpolation_on** Enables overlay image interpolation.

-  **-overlay.interpolation_off** Disables overlay image interpolation.

-  **-overlay.colourmap index** Sets the colourmap of the overlay as indexed in the colourmap dropdown menu.

ROI editor tool options
^^^^^^^^^^^^^^^^^^^^^^^

-  **-roi.load image** Loads the specified image on the ROI editor tool.

-  **-roi.opacity value** Sets the overlay opacity to floating value [0-1].

Tractography tool options
^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-tractography.load tracks** Load the specified tracks file into the tractography tool.

ODF tool options
^^^^^^^^^^^^^^^^

-  **-odf.load_sh image** Loads the specified SH-based ODF image on the ODF tool.

-  **-odf.load_tensor image** Loads the specified tensor image on the ODF tool.

-  **-odf.load_dixel image** Loads the specified dixel-based image on the ODF tool.

Vector plot tool options
^^^^^^^^^^^^^^^^^^^^^^^^

-  **-vector.load image** Load the specified MRtrix sparse image file (.msf) into the fixel tool.

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

-  **-quiet** do not display information messages or progress status.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

-  **-failonwarn** terminate program if a warning is produced

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

Tournier, J.-D.; Calamante, F. & Connelly, A. MRtrix: Diffusion tractography in crossing fiber regions. Int. J. Imaging Syst. Technol., 2012, 22, 53-66

--------------



**Author:** J-Donald Tournier (d.tournier@brain.org.au), Dave Raffelt (david.raffelt@florey.edu.au) and Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

