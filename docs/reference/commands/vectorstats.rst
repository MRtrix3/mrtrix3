.. _vectorstats:

vectorstats
===========

Synopsis
--------

::

    vectorstats [ options ]  input design contrast output

-  *input*: a text file listing the file names of the input subject data
-  *design*: the design matrix. Note that a column of 1's will need to be added for correlations.
-  *contrast*: the contrast vector, specified as a single row of weights
-  *output*: the filename prefix for all output.

Description
-----------

Statistical testing of vector data using non-parametric permutation testing.

Options
-------

Options for permutation testing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-notest** don't perform permutation testing and only output population statistics (effect size, stdev etc)

-  **-nperms num** the number of permutations (Default: 5000)

-  **-permutations file** manually define the permutations (relabelling). The input should be a text file defining a m x n matrix, where each relabelling is defined as a column vector of size    m, and the number of columns, n, defines the number of permutations. Can be generated with the palm_quickperms function in PALM (http://fsl.fmrib.ox.ac.uk/fsl/fslwiki/PALM). Overrides the nperms option.

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

