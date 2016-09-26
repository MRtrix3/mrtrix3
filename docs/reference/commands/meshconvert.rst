.. _meshconvert:

meshconvert
===========

Synopsis
--------

::

    meshconvert [ options ]  input output

-  *input*: the input mesh file
-  *output*: the output mesh file

Description
-----------

convert meshes between different formats, and apply transformations.

Options
-------

-  **-binary** write the output file in binary format

Options for applying spatial transformations to vertices
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-transform_first2real image** transform vertices from FSL FIRST's native corrdinate space to real space

-  **-transform_real2first image** transform vertices from FSL real space to FIRST's native corrdinate space

-  **-transform_voxel2real image** transform vertices from voxel space to real space

-  **-transform_real2voxel image** transform vertices from real space to voxel space

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



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

