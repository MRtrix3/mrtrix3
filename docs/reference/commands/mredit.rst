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

Options
-------

-  **-plane axis coord value** fill one or more planes on a particular image axis

-  **-sphere position radius value** draw a sphere with radius in mm

-  **-voxel position value** change the image value within a single voxel

-  **-scanner** indicate that coordinates are specified in scanner space, rather than as voxel coordinates

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2019 the MRtrix3 contributors.

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


