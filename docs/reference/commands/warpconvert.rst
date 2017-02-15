.. _warpconvert:

warpconvert
===========

Synopsis
--------

Convert between different representations of a non-linear warp

Usage
--------

::

    warpconvert [ options ]  in out

-  *in*: the input warp image.
-  *out*: the output warp image.

Description
-----------

A deformation field is defined as an image where each voxel defines the corresponding position in the other image (in scanner space coordinates). A displacement field stores the displacements (in mm) to the other image from the each voxel's position (in scanner space). The warpfull file is the 5D format output from mrregister -nl_warp_full, which contains linear transforms, warps and their inverses that map each image to a midway space.

Options
-------

-  **-type choice** the conversion type required. Valid choices are: deformation2displacement, displacement2deformation, warpfull2deformation, warpfull2displacement (Default: deformation2displacement)

-  **-template image** define a template image when converting a warpfull file (which is defined on a grid in the midway space between image 1 & 2). For example to generate the deformation field that maps image1 to image2, then supply image2 as the template image

-  **-midway_space** to be used only with warpfull2deformation and warpfull2displacement conversion types. The output will only contain the non-linear warp to map an input image to the midway space (defined by the warpfull grid). If a linear transform exists in the warpfull file header then it will be composed and included in the output.

-  **-from image** to be used only with warpfull2deformation and warpfull2displacement conversion types. Used to define the direction of the desired output field.Use -from 1 to obtain the image1->image2 field and from 2 for image2->image1. Can be used in combination with the -midway_space option to produce a field that only maps to midway space.

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

