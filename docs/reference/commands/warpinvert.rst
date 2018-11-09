.. _warpinvert:

warpinvert
===================

Synopsis
--------

Invert a non-linear warp field

Usage
--------

::

    warpinvert [ options ]  in out

-  *in*: the input warp image.
-  *out*: the output warp image.

Description
-----------

By default, this command assumes that the input warp field is a deformation field, i.e. each voxel stores the corresponding position in the other image (in scanner space), and the calculated output warp image will also be a deformation field. If the input warp field is instead a displacment field, i.e. where each voxel stores an offset from which to sample the other image (but still in scanner space), then the -displacement option should be used; the output warp field will additionally be calculated as a displacement field in this case.

Options
-------

-  **-template image** define a template image grid for the output warp

-  **-displacement** indicates that the input warp field is a displacement field; the output will also be a displacement field

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



**Author:** Robert E. Smith (robert.smith@florey.edu.au) and David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (c) 2008-2018 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/

MRtrix3 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/


