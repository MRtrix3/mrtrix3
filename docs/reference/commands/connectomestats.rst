.. _connectomestats:

connectomestats
===========

Synopsis
--------

::

    connectomestats [ options ]  input algorithm design contrast output

-  *input*: a text file listing the file names of the input connectomes
-  *algorithm*: the algorithm to use in network-based clustering/enhancement. Options are: nbs, nbs_tfce, none
-  *design*: the design matrix. Note that a column of 1's will need to be added for correlations.
-  *contrast*: the contrast vector, specified as a single row of weights
-  *output*: the filename prefix for all output.

Description
-----------

Connectome group-wise statistics at the edge level using non-parametric permutation testing.

Options
-------

Options for permutation testing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-notest** don't perform permutation testing and only output population statistics (effect size, stdev etc)

-  **-nperms num** the number of permutations (Default: 5000)

-  **-nonstationary** perform non-stationarity correction

-  **-nperms_nonstationary num** the number of permutations used when precomputing the empirical statistic image for nonstationary correction (Default: 5000)

Options for controlling TFCE behaviour
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-tfce_dh value** the height increment used in the tfce integration (default: 0.1)

-  **-tfce_e value** tfce extent exponent (default: 0.4)

-  **-tfce_h value** tfce height exponent (default: 3)

Additional options for connectomestats
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-threshold value** the t-statistic value to use in threshold-based clustering algorithms

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

* If using the NBS algorithm: Zalesky, A.; Fornito, A. & Bullmore, E. T. Network-based statistic: Identifying differences in brain networks. NeuroImage, 2010, 53, 1197-1207

* If using the NBS-TFCE algorithm: Vinokur, L.; Zalesky, A.; Raffelt, D.; Smith, R.E. & Connelly, A. A Novel Threshold-Free Network-Based Statistics Method: Demonstration using Simulated Pathology. OHBM, 2015, 4144

* If using the -nonstationary option: Salimi-Khorshidi, G.; Smith, S.M. & Nichols, T.E. Adjusting the effect of nonstationarity in cluster-based and TFCE inference. Neuroimage, 2011, 54(3), 2006-19

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

