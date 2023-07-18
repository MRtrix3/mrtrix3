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



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

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


