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

-  *input*: the input transforms (either linear or non-linear warps).
-  *output*: the output file (may be a linear transformation text file, or a deformation warp field image, depending on usage)

Description
-----------

Any linear transforms must be supplied as a 4x4 matrix in a text file (e.g. as per the output of mrregister). Any warp fields must be supplied as a 4D image representing a deformation field (e.g. as output from mrrregister -nl_warp).

Input transformations should be provided to the command in the order in which they would be applied to an image if they were to be applied individually.

If all input transformations are linear, and the -template option is not provided, then the file output by the command will also be a linear transformation saved as a 4x4 matrix in a text file. If a template image is supplied, then the output will always be a deformation field. If at least one of the inputs is a warp field, then the output will be a deformation field, which will be defined on the grid of the last input warp image supplied if the -template option is not used.

Options
-------

-  **-template image** define the output grid defined by a template image

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



**Author:** David Raffelt (david.raffelt@florey.edu.au)

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


