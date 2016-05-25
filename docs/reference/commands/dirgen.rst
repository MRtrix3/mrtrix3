.. _dirgen:

dirgen
===========

Synopsis
--------

::

    dirgen [ options ]  ndir dirs

-  *ndir*: the number of directions to generate.
-  *dirs*: the text file to write the directions to, as [ az el ] pairs.

Description
-----------

generate a set of uniformly distributed directions using a bipolar electrostatic repulsion model.

Options
-------

-  **-power exp** specify exponent to use for repulsion power law (default: 2). This must be a power of 2 (i.e. 2, 4, 8, 16, ...).

-  **-niter num** specify the maximum number of iterations to perform (default: 10000).

-  **-unipolar** optimise assuming a unipolar electrostatic repulsion model rather than the bipolar model normally assumed in DWI

-  **-cartesian** Output the directions in Cartesian coordinates [x y z] instead of [az el].

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

Jones, D.; Horsfield, M. & Simmons, A. Optimal strategies for measuring diffusion in anisotropic systems by magnetic resonance imaging. Magnetic Resonance in Medicine, 1999, 42: 515-525

Papadakis, N. G.; Murrills, C. D.; Hall, L. D.; Huang, C. L.-H. & Adrian Carpenter, T. Minimal gradient encoding for robust estimation of diffusion anisotropy. Magnetic Resonance Imaging, 2000, 18: 671-679

--------------



**Author:** J-Donald Tournier (jdtournier@gmail.com)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

