.. _transformcalc:

transformcalc
===================

Synopsis
--------

Perform calculations on linear transformation matrices

Usage
--------

::

    transformcalc [ options ]  input [ input ... ] operation output

-  *input*: the input for the specified operation
-  *operation*: the operation to perform, one of: invert, half, rigid, header, average, interpolate, decompose.invert: invert the input transformation:matrix_in invert outputhalf: calculate the matrix square root of the input transformation:matrix_in half outputrigid: calculate the rigid transformation of the affine input transformation:matrix_in rigid outputheader: calculate the transformation matrix from an original image and an image with modified header:mov mapmovhdr header outputaverage: calculate the average affine matrix of all input matrices:input ... average outputinterpolate: create interpolated transformation matrix between input (t=0) and input2 (t=1). Based on matrix decomposition with linear interpolation of  translation, rotation and stretch described in  Shoemake, K., Hill, M., & Duff, T. (1992). Matrix Animation and Polar Decomposition.  Matrix, 92, 258-264. doi:10.1.1.56.1336input input2 interpolate outputdecompose: decompose transformation matrix M into translation, rotation and stretch and shear (M = T * R * S). The output is a key-value text file scaling: vector of 3 scaling factors in x, y, z direction, shear: list of shear factors for xy, xz, yz axes, angles: list of Euler angles about static x, y, z axes in radians in the range [0:pi]x[-pi:pi]x[-pi:pi], angle_axis: angle in radians and rotation axis, translation : translation vector along x, y, z axes in mm, R: composed roation matrix (R = rot_x * rot_y * rot_z), S: composed scaling and shear matrix.matrix_in decompose output
-  *output*: the output transformation matrix.

Options
-------

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



**Author:** Max Pietsch (maximilian.pietsch@kcl.ac.uk)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.

