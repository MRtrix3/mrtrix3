.. _transformcalc:

transformcalc
===========

Synopsis
--------

::

    transformcalc [ options ]  input [ input ... ] operation output

-  *input*: the input for the specified operation
-  *operation*: the operation to perform, one of: flirt_import, invert, half, rigid, header, average, interpolate.flirt_import: Convert a transformation matrix produced by FSL's flirt command into a format usable by MRtrix. You'll need to provide as additional arguments the save NIfTI images that were passed to flirt with the -in and -ref options:matrix_in in ref flirt_import outputinvert: invert the input transformation:matrix_in invert outputhalf: calculate the matrix square root of the input transformation:matrix_in half outputrigid: calculate the rigid transformation of the affine input transformation:matrix_in rigid outputheader: calculate the transformation matrix from an original image and an image with modified header:mov mapmovhdr header outputaverage: calculate the average affine matrix of all input matrices:input ... average outputinterpolate: create interpolated transformation matrix between input (t=0) and input2 (t=1). Based on matrix decomposition with linear interpolation of  translation, rotation and stretch described in  Shoemake, K., Hill, M., & Duff, T. (1992). Matrix Animation and Polar Decomposition.  Matrix, 92, 258-264. doi:10.1.1.56.1336input input2 interpolate output
-  *output*: the output transformation matrix.

Description
-----------

This command's function is to process linear transformation matrices.

It allows to perform affine matrix operations or to convert the transformation matrix provided by FSL's flirt command to a format usable in MRtrix

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

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

