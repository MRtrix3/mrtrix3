.. _meshvalidate:

meshvalidate
===================

Synopsis
--------

Validate a mesh surface file

Usage
--------

::

    meshvalidate [ options ]  mesh

-  *mesh*: the input mesh file

Description
-----------

This command checks that a mesh surface file conforms to the requirements of a valid closed orientable surface. The following properties are verified:

1. No disconnected vertices: every vertex in the file must be referenced by at least one polygon.

2. No duplicate vertices: no two vertices may occupy the same 3D position.

3. No duplicate polygons: no two polygons may reference the same set of vertex indices.

4. Closed surface: every edge must be shared by exactly two polygons. An edge belonging to only one polygon indicates a boundary (open surface); an edge shared by three or more polygons indicates a non-manifold mesh.

5. Single connected component: all polygons must belong to a single connected piece. A surface with multiple disconnected pieces is not valid.

6. Consistent normal orientation: for every shared edge, the two adjacent polygons must traverse it in opposite directions. If both polygons traverse the edge in the same direction their winding orders — and therefore their surface normals — are inconsistent.

Options
-------

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status; alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages & debug input data.

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

**Copyright:** Copyright (c) 2008-2026 the MRtrix3 contributors.

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


