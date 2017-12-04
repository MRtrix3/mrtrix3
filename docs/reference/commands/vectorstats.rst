.. _vectorstats:

vectorstats
===================

Synopsis
--------

Statistical testing of vector data using non-parametric permutation testing

Usage
--------

::

    vectorstats [ options ]  input design contrast output

-  *input*: a text file listing the file names of the input subject data
-  *design*: the design matrix
-  *contrast*: the contrast matrix
-  *output*: the filename prefix for all output

Description
-----------

This command can be used to perform permutation testing of any form of data. The data for each input subject must be stored in a text file, with one value per row. The data for each row across subjects will be tested independently, i.e. there is no statistical enhancement that occurs between the data; however family-wise error control will be used.

In some software packages, a column of ones is automatically added to the GLM design matrix; the purpose of this column is to estimate the "global intercept", which is the predicted value of the observed variable if all explanatory variables were to be zero. However there are rare situations where including such a column would not be appropriate for a particular experimental design. Hence, in MRtrix3 statistical inference commands, it is up to the user to determine whether or not this column of ones should be included in their design matrix, and add it explicitly if necessary. The contrast matrix must also reflect the presence of this additional column.

Options
-------

Options relating to shuffling of data for nonparametric statistical inference
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-errors spec** specify nature of errors for shuffling; options are: ee,ise,both (default: ee)

-  **-nshuffles number** the number of shuffles (default: 5000)

-  **-permutations file** manually define the permutations (relabelling). The input should be a text file defining a m x n matrix, where each relabelling is defined as a column vector of size m, and the number of columns, n, defines the number of permutations. Can be generated with the palm_quickperms function in PALM (http://fsl.fmrib.ox.ac.uk/fsl/fslwiki/PALM). Overrides the -nshuffles option.

-  **-signflips file** manually define the signflips

Options related to the General Linear Model (GLM)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-column path** add a column to the design matrix corresponding to subject element-wise values (note that the contrast matrix must include an additional column for each use of this option); the text file provided via this option should contain a file name for each subject

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

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.

