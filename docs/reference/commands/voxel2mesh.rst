.. _voxel2mesh:

voxel2mesh
===================

Synopsis
--------

Generate a surface mesh representation from a voxel image

Usage
--------

::

    voxel2mesh [ options ]  input output

-  *input*: the input image.
-  *output*: the output mesh file.

Description
-----------

This command utilises the Marching Cubes algorithm to generate a polygonal surface that represents the isocontour(s) of the input image at a particular intensity. By default, an appropriate threshold will be determined automatically from the input image, however the intensity value of the isocontour(s) can instead be set manually using the -threhsold option.

If the -blocky option is used, then the Marching Cubes algorithm will not be used. Instead, the input image will be interpreted as a binary mask image, and polygonal surfaces will be generated at the outer faces of the voxel clusters within the mask.

Options
-------

-  **-blocky** generate a 'blocky' mesh that precisely represents the voxel edges

-  **-threshold value** manually set the intensity threshold for the Marching Cubes algorithm

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



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2018 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/

MRtrix3 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/


