.. _mrpad:

mrpad
===========

Synopsis
--------

::

    mrpad [ options ]  image_in image_out

-  *image_in*: the image to be padded
-  *image_out*: the output path for the resulting padded image

Description
-----------

Pad an image to increase the FOV

Options
-------

-  **-uniform number** pad the input image by a uniform number of voxels on all sides (in 3D)

-  **-axis index lower upper** pad the input image along the provided axis (defined by index). Lower and upper define the number of voxels to add to the lower and upper bounds of the axis

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

--------------



**Author:** David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.

