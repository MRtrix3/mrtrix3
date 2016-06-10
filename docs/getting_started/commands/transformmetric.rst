transformmetric
===========

Synopsis
--------

::

    transformmetric [ options ]  image

-  *image*: the image that defines the space over which the dissimilarity is measured.

Description
-----------

computes the dissimiliarty metric between two transformations. Currently only affine transformations and the mean absolute displacement metric are implemented

Options
-------

-  **-linear transform** specify a 4x4 linear transform to apply, in the form of a 4x4 ascii file. Note the standard 'reverse' convention is used, where the transform maps points in the template image to the moving image.

-  **-linear_inverse** invert linear transformation

-  **-linear2 transform** TODO: compare transformations

-  **-template transformation** use the voxel to scanner transformation of the template image instead of the linear transformation. 

-  **-voxel** normalise dissimiliarity to voxel size of image

-  **-cumulative_absolute** sum the absolute of each voxel position displacement

-  **-norm** display the norm of the displacement.

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications

-  **-failonwarn** terminate program if a warning is produced

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------



**Author:** Maximilian Pietsch (maximilian.pietsch@kcl.ac.uk)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

