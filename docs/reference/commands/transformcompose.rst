.. _transformcompose:

transformcompose
===================

Synopsis
--------

Compose any number of linear transformations and/or warps into a single transformation

Usage
--------

::

    transformcompose [ options ]  input [ input ... ] output

-  *input*: the input transforms (either linear or non-linear warps). List transforms in the order you like them to be applied to an image (as if you were applying them seperately with mrtransform).
-  *output*: the output file. If all input transformations are linear, then the output will also be a linear transformation saved as a 4x4 matrix in a text file. If a template image is supplied, then the output will always be a deformation field (see below). If all inputs are warps, or a mix of linear and warps, then the output will be a deformation field defined on the grid of the last input warp supplied.

Description
-----------

The input linear transforms must be supplied in as a 4x4 matrix in a text file (as per the output of mrregister).The input warp fields must be supplied as a 4D image representing a deformation field (as output from mrrregister -nl_warp).

Options
-------

-  **-template image** define the output grid defined by a template image

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



**Author:** David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (c) 2008-2018 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/

MRtrix3 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/


