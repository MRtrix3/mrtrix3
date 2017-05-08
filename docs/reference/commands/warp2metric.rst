.. _warp2metric:

warp2metric
===================

Synopsis
--------

Compute fixel-wise or voxel-wise metrics from a 4D deformation field

Usage
--------

::

    warp2metric [ options ]  in

-  *in*: the input deformation field

Options
-------

-  **-fc template_fixel_directory output_fixel_directory output_fixel_data** use an input template fixel image to define fibre orientations and output a fixel image describing the change in fibre cross-section (FC) in the perpendicular plane to the fixel orientation. e.g. warp2metric warp.mif -fc fixel_template_directory output_fixel_directory fc.mif

-  **-jmat output** output a Jacobian matrix image stored in column-major order along the 4th dimension.Note the output jacobian describes the warp gradient w.r.t the scanner space coordinate system

-  **-jdet output** output the Jacobian determinant instead of the full matrix

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

References
^^^^^^^^^^

Raffelt, D.; Tournier, JD/; Smith, RE.; Vaughan, DN.; Jackson, G.; Ridgway, GR. Connelly, A.Investigating White Matter Fibre Density and Morphology using Fixel-Based Analysis. Neuroimage, 2016, 10.1016/j.neuroimage.2016.09.029

--------------



**Author:** David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


