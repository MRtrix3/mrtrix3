.. _connectomestats:

connectomestats
===================

Synopsis
--------

Connectome group-wise statistics at the edge level using non-parametric permutation testing

Usage
--------

::

    connectomestats [ options ]  input algorithm design contrast output

-  *input*: a text file listing the file names of the input connectomes
-  *algorithm*: the algorithm to use in network-based clustering/enhancement. Options are: nbs, tfnbs, none
-  *design*: the design matrix
-  *contrast*: the contrast matrix
-  *output*: the filename prefix for all output.

Description
-----------

For the TFNBS algorithm, default parameters for statistical enhancement have been set based on the work in:  |br|
Vinokur, L.; Zalesky, A.; Raffelt, D.; Smith, R.E. & Connelly, A. A Novel Threshold-Free Network-Based Statistics Method: Demonstration using Simulated Pathology. OHBM, 2015, 4144;  |br|
and:  |br|
Vinokur, L.; Zalesky, A.; Raffelt, D.; Smith, R.E. & Connelly, A. A novel threshold-free network-based statistical method: Demonstration and parameter optimisation using in vivo simulated pathology. In Proc ISMRM, 2015, 2846.  |br|
Note however that not only was the optimisation of these parameters not very precise, but the outcomes of statistical inference (for both this algorithm and the NBS method) can vary markedly for even small changes to enhancement parameters. Therefore the specificity of results obtained using either of these methods should be interpreted with caution.

In some software packages, a column of ones is automatically added to the GLM design matrix; the purpose of this column is to estimate the "global intercept", which is the predicted value of the observed variable if all explanatory variables were to be zero. However there are rare situations where including such a column would not be appropriate for a particular experimental design. Hence, in MRtrix3 statistical inference commands, it is up to the user to determine whether or not this column of ones should be included in their design matrix, and add it explicitly if necessary. The contrast matrix must also reflect the presence of this additional column.

Options
-------

Options relating to shuffling of data for nonparametric statistical inference
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-notest** don't perform statistical inference; only output population statistics (effect size, stdev etc)

-  **-errors spec** specify nature of errors for shuffling; options are: ee,ise,both (default: ee)

-  **-exchange_within file** specify blocks of observations within each of which data may undergo restricted exchange

-  **-exchange_whole file** specify blocks of observations that may be exchanged with one another (for independent and symmetric errors, sign-flipping will occur block-wise)

-  **-strong** use strong familywise error control across multiple hypotheses

-  **-nshuffles number** the number of shuffles (default: 5000)

-  **-permutations file** manually define the permutations (relabelling). The input should be a text file defining a m x n matrix, where each relabelling is defined as a column vector of size m, and the number of columns, n, defines the number of permutations. Can be generated with the palm_quickperms function in PALM (http://fsl.fmrib.ox.ac.uk/fsl/fslwiki/PALM). Overrides the -nshuffles option.

-  **-nonstationarity** perform non-stationarity correction

-  **-skew_nonstationarity value** specify the skew parameter for empirical statistic calculation (default for this command is 1)

-  **-nshuffles_nonstationarity number** the number of shuffles to use when precomputing the empirical statistic image for non-stationarity correction (default: 5000)

-  **-permutations_nonstationarity file** manually define the permutations (relabelling) for computing the emprical statistics for non-stationarity correction. The input should be a text file defining a m x n matrix, where each relabelling is defined as a column vector of size m, and the number of columns, n, defines the number of permutations. Can be generated with the palm_quickperms function in PALM (http://fsl.fmrib.ox.ac.uk/fsl/fslwiki/PALM) Overrides the -nshuffles_nonstationarity option.

Options for controlling TFCE behaviour
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-tfce_dh value** the height increment used in the tfce integration (default: 0.1)

-  **-tfce_e value** tfce extent exponent (default: 0.4)

-  **-tfce_h value** tfce height exponent (default: 3)

Options related to the General Linear Model (GLM)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-variance file** define variance groups for the G-statistic; measurements for which the expected variance is equivalent should contain the same index

-  **-ftests path** perform F-tests; input text file should contain, for each F-test, a row containing ones and zeros, where ones indicate the rows of the contrast matrix to be included in the F-test.

-  **-fonly** only assess F-tests; do not perform statistical inference on entries in the contrast matrix

-  **-column path** *(multiple uses permitted)* add a column to the design matrix corresponding to subject edge-wise values (note that the contrast matrix must include an additional column for each use of this option); the text file provided via this option should contain a file name for each subject

Additional options for connectomestats
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-threshold value** the t-statistic value to use in threshold-based clustering algorithms

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

* If using the NBS algorithm:  |br|
  Zalesky, A.; Fornito, A. & Bullmore, E. T. Network-based statistic: Identifying differences in brain networks.  |br|
  NeuroImage, 2010, 53, 1197-1207

* If using the TFNBS algorithm:  |br|
  Baggio, H.C.; Abos, A.; Segura, B.; Campabadal, A.; Garcia-Diaz, A.; Uribe, C.; Compta, Y.; Marti, M.J.; Valldeoriola, F.; Junque, C. Statistical inference in brain graphs using threshold-free network-based statistics.HBM, 2018, 39, 2289-2302

* If using the -nonstationary option:  |br|
  Salimi-Khorshidi, G.; Smith, S.M. & Nichols, T.E. Adjusting the effect of nonstationarity in cluster-based and TFCE inference.  |br|
  Neuroimage, 2011, 54(3), 2006-19

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


