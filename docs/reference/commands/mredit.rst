.. _mredit:

mredit
===================

Synopsis
--------

Directly edit the intensities within an image from the command-line

Usage
--------

::

    mredit [ options ]  input[ output ]

-  *input*: the input image
-  *output*: the (optional) output image

Description
-----------

A range of options are provided to enable direct editing of voxel intensities based on voxel / real-space coordinates. If only one image path is provided, the image will be edited in-place (use at own risk); if input and output image paths are provided, the output will contain the edited image, and the original image will not be modified in any way.

The geometric primitives -sphere, -ellipsoid, -cuboid and -line are by default defined with respect to the image voxel grid: positions are voxel indices, sizes (radii and side lengths) are measured in voxels, and the principal axes of each shape are aligned with the image axes. If the -scanner option is used, these quantities are instead defined with respect to scanner space: positions are scanner-space coordinates in millimetres, sizes are measured in millimetres, and the principal axes of each shape are aligned with the scanner axes. For an image whose voxel axes are not aligned with the scanner axes, or whose voxels are not isotropic, the two interpretations can yield substantially different results.

Options
-------

-  **-plane axis coord value** *(multiple uses permitted)* fill one or more planes on a particular image axis

-  **-sphere position radius value** *(multiple uses permitted)* draw a sphere of the specified radius

-  **-ellipsoid position radii value** *(multiple uses permitted)* draw an ellipsoid (a single radius value yields a sphere)

-  **-cuboid position size value** *(multiple uses permitted)* draw a rectangular cuboid (a single side-length value yields a cube)

-  **-line first second radius value** *(multiple uses permitted)* draw a straight line of the specified radius between two points (i.e. a cylinder with hemispherical caps)

-  **-voxel position value** *(multiple uses permitted)* change the image value within a single voxel

-  **-scanner** interpret all stencil positions, sizes and orientations in scanner space (mm), rather than with respect to the voxel grid

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status; alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages & debug input data.

-  **-force** force overwrite of output files (caution: using the same file as input and output might cause unexpected behaviour).

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-config key value** *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2026 the MRtrix3 contributors.

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


