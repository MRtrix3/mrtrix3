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

All editing options are by default interpreted with respect to the image voxel grid: -plane fills a whole image plane perpendicular to one image axis; -voxel addresses a single voxel by its indices; and the geometric primitives -sphere, -ellipsoid, -cuboid and -line take positions as voxel indices, sizes (radii and side lengths) measured in voxels, and principal axes aligned with the image axes. If the -scanner option is used, these inputs are instead interpreted with respect to scanner space: -plane fills a digital plane perpendicular to one scanner axis at a given offset in millimetres; -voxel modifies the single voxel that contains the specified scanner-space position; positions are scanner-space coordinates in millimetres; sizes are measured in millimetres; and the principal axes of each shape are aligned with the scanner axes. For an image whose voxel axes are not aligned with the scanner axes, or whose voxels are not isotropic, the two interpretations can yield substantially different results.

Unlike most MRtrix3 commands, the order in which editing options are provided on the command-line is significant: each stencil is applied to the image in turn, in the order specified, so that wherever two stencils overlap the one provided later takes precedence.

Options
-------

-  **-plane axis coord value** *(multiple uses permitted)* fill one or more planes perpendicular to the specified axis |br|
   *axis*: the axis perpendicular to the plane(s) |br|
   *coord*: the coordinate(s) along that axis at which to fill |br|
   *value*: the intensity value to set

-  **-sphere position radius value** *(multiple uses permitted)* draw a sphere of the specified radius |br|
   *position*: the centre of the sphere |br|
   *radius*: the radius of the sphere |br|
   *value*: the intensity value to set

-  **-ellipsoid position radii value** *(multiple uses permitted)* draw an ellipsoid (a single radius value yields a sphere) |br|
   *position*: the centre of the ellipsoid |br|
   *radii*: the radii of the ellipsoid |br|
   *value*: the intensity value to set

-  **-cuboid position size value** *(multiple uses permitted)* draw a rectangular cuboid (a single side-length value yields a cube) |br|
   *position*: the centre of the cuboid |br|
   *size*: the side length(s) of the cuboid |br|
   *value*: the intensity value to set

-  **-line first second value** *(multiple uses permitted)* draw a single-voxel-thick line between two points using Bresenham's algorithm |br|
   *first*: the first end-point of the line |br|
   *second*: the second end-point of the line |br|
   *value*: the intensity value to set

-  **-voxel position value** *(multiple uses permitted)* change the image value within a single voxel |br|
   *position*: the position of the voxel |br|
   *value*: the intensity value to set

-  **-scanner** interpret all stencil positions, sizes and orientations in scanner space (mm), rather than with respect to the voxel grid

Standard options
^^^^^^^^^^^^^^^^

-  **-force** force overwrite of output files (caution: using the same file as input and output might cause unexpected behaviour).

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-config key value** *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

Verbosity options
"""""""""""""""""

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status; alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages & debug input data.

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


