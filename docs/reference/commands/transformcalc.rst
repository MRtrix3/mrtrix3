.. _transformcalc:

transformcalc
===================

Synopsis
--------

Perform calculations on linear transformation matrices

Usage
--------

::

    transformcalc [ options ]  inputs [ inputs ... ] operation output

-  *inputs*: the input(s) for the specified operation
-  *operation*: the operation to perform, one of: invert, half, rigid, header, average, interpolate, decompose, align_vertices_rigid, align_vertices_rigid_scale (see description section for details).
-  *output*: the output transformation matrix.

Example usages
--------------

-   *Invert a transformation*::

        $ transformcalc matrix_in.txt invert matrix_out.txt

-   *Calculate the matrix square root of the input transformation (halfway transformation)*::

        $ transformcalc matrix_in.txt half matrix_out.txt

-   *Calculate the rigid component of an affine input transformation*::

        $ transformcalc affine_in.txt rigid rigid_out.txt

-   *Calculate the transformation matrix from an original image and an image with modified header*::

        $ transformcalc mov mapmovhdr header output

-   *Calculate the average affine matrix of a set of input matrices*::

        $ transformcalc input1.txt ... inputN.txt average matrix_out.txt

-   *Create interpolated transformation matrix between two inputs*::

        $ transformcalc input1.txt input2.txt interpolate matrix_out.txt

    Based on matrix decomposition with linear interpolation of translation, rotation and stretch described in: Shoemake, K., Hill, M., & Duff, T. (1992). Matrix Animation and Polar Decomposition. Matrix, 92, 258-264. doi:10.1.1.56.1336

-   *Decompose transformation matrix M into translation, rotation and stretch and shear (M = T * R * S)*::

        $ transformcalc matrix_in.txt decompose matrixes_out.txt

    The output is a key-value text file containing: scaling: vector of 3 scaling factors in x, y, z direction; shear: list of shear factors for xy, xz, yz axes; angles: list of Euler angles about static x, y, z axes in radians in the range [0:pi]x[-pi:pi]x[-pi:pi]; angle_axis: angle in radians and rotation axis; translation : translation vector along x, y, z axes in mm; R: composed roation matrix (R = rot_x * rot_y * rot_z); S: composed scaling and shear matrix

-   *Calculate transformation that aligns two images based on sets of corresponding landmarks*::

        $ transformcalc input moving.txt fixed.txt align_vertices_rigid rigid.txt

    Similary, 'align_vertices_rigid_scale' produces an affine matrix (rigid and global scale). Vertex coordinates are in scanner space, corresponding vertices must be stored in the same row of moving.txt and fixed.txt. Requires 3 or more vertices in each file. Algorithm: Kabsch 'A solution for the best rotation to relate two sets of vectors' DOI:10.1107/S0567739476001873

Options
-------

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status; alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files (caution: using the same file as input and output might cause unexpected behaviour).

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-config key value** *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Max Pietsch (maximilian.pietsch@kcl.ac.uk)

**Copyright:** Copyright (c) 2008-2023 the MRtrix3 contributors.

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


