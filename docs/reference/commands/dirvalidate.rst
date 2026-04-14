.. _dirvalidate:

dirvalidate
===================

Synopsis
--------

Validate a direction set file

Usage
--------

::

    dirvalidate [ options ]  dirs

-  *dirs*: the input direction set text file

Description
-----------

This command checks that a direction set text file is well-formed. The format is inferred from the number of columns:

2 columns: spherical coordinates (azimuth, inclination). The azimuth must lie strictly within [-2π, 2π], and the difference between maximal and minimal values must not exceed 2π. The inclination must lie within [-π, π], and the difference between maximal and minimal values must not exceed π.

3 columns: Cartesian unit directions (x, y, z). All three components must lie within [-1, 1]. For most files, the Euclidean norm of each direction will be 1.0; for others some quantitative value may be encoded by having some directions with a norm less than unity. The command reports to the console which of these conventions the input file appears to conform to.

4 columns: diffusion gradient table (x, y, z, b-value). All direction components must lie within [-1, 1]. The b-value must be non-negative. For non-zero b-value entries, the gradient direction must be of unit norm. b=0 entries may carry a zero direction vector.

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


