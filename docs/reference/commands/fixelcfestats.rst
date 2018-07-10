.. _fixelcfestats:

fixelcfestats
===================

Synopsis
--------

Fixel-based analysis using connectivity-based fixel enhancement and non-parametric permutation testing

Usage
--------

::

    fixelcfestats [ options ]  in_fixel_directory subjects design contrast tracks out_fixel_directory

-  *in_fixel_directory*: the fixel directory containing the data files for each subject (after obtaining fixel correspondence
-  *subjects*: a text file listing the subject identifiers (one per line). This should correspond with the filenames in the fixel directory (including the file extension), and be listed in the same order as the rows of the design matrix.
-  *design*: the design matrix
-  *contrast*: the contrast matrix, specified as rows of weights
-  *tracks*: the tracks used to determine fixel-fixel connectivity
-  *out_fixel_directory*: the output directory where results will be saved. Will be created if it does not exist

Description
-----------

In some software packages, a column of ones is automatically added to the GLM design matrix; the purpose of this column is to estimate the "global intercept", which is the predicted value of the observed variable if all explanatory variables were to be zero. However there are rare situations where including such a column would not be appropriate for a particular experimental design. Hence, in MRtrix3 statistical inference commands, it is up to the user to determine whether or not this column of ones should be included in their design matrix, and add it explicitly if necessary. The contrast matrix must also reflect the presence of this additional column.

Note that if the -mask option is used, the output fixel directory will still contain the same set of fixels as that present in the input fixel template, in order to retain fixel correspondence. However a consequence of this is that all fixels in the template will be initialy visible when the output fixel directory is loaded in mrview. Those fixels outside the processing mask will immediately disappear from view as soon as any data-file-based fixel colouring or thresholding is applied.

Options
-------

Options relating to shuffling of data for nonparametric statistical inference
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-errors spec** specify nature of errors for shuffling; options are: ee,ise,both (default: ee)

-  **-nshuffles number** the number of shuffles (default: 5000)

-  **-permutations file** manually define the permutations (relabelling). The input should be a text file defining a m x n matrix, where each relabelling is defined as a column vector of size m, and the number of columns, n, defines the number of permutations. Can be generated with the palm_quickperms function in PALM (http://fsl.fmrib.ox.ac.uk/fsl/fslwiki/PALM). Overrides the -nshuffles option.

-  **-signflips file** manually define the signflips

-  **-nonstationarity** perform non-stationarity correction

-  **-skew value** specify the skew parameter for empirical statistic calculation (default for this command is 0.5)

-  **-nshuffles_nonstationarity number** the number of shuffles to use when precomputing the empirical statistic image for non-stationarity correction (default: 5000)

-  **-permutations_nonstationarity file** manually define the permutations (relabelling) for computing the emprical statistics for non-stationarity correction. The input should be a text file defining a m x n matrix, where each relabelling is defined as a column vector of size m, and the number of columns, n, defines the number of permutations. Can be generated with the palm_quickperms function in PALM (http://fsl.fmrib.ox.ac.uk/fsl/fslwiki/PALM) Overrides the -nshuffles_nonstationarity option.

-  **-signflips_nonstationarity file** manually define the signflips for computing the empirical statistics for non-stationarity correction

-  **-fast_nonstationarity** perform a fast non-stationarity correction that does not require permutations

Parameters for the Connectivity-based Fixel Enhancement algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-cfe_dh value** the height increment used in the cfe integration (default: 0.1)

-  **-cfe_e value** cfe extent exponent (default: 2)

-  **-cfe_h value** cfe height exponent (default: 3)

-  **-cfe_c value** cfe connectivity exponent (default: 0.5)

Options related to the General Linear Model (GLM)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-ftests path** perform F-tests; input text file should contain, for each F-test, a row containing ones and zeros, where ones indicate the rows of the contrast matrix to be included in the F-test.

-  **-fonly** only assess F-tests; do not perform statistical inference on entries in the contrast matrix

-  **-column path** add a column to the design matrix corresponding to subject fixel-wise values (note that the contrast matrix must include an additional column for each use of this option); the text file provided via this option should contain a file name for each subject

Additional options for fixelcfestats
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-smooth FWHM** smooth the fixel value along the fibre tracts using a Gaussian kernel with the supplied FWHM (default: 10mm)

-  **-connectivity threshold** a threshold to define the required fraction of shared connections to be included in the neighbourhood (default: 0.01)

-  **-angle value** the max angle threshold for assigning streamline tangents to fixels (Default: 45 degrees)

-  **-mask file** provide a fixel data file containing a mask of those fixels to be used during processing

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

Raffelt, D.; Smith, RE.; Ridgway, GR.; Tournier, JD.; Vaughan, DN.; Rose, S.; Henderson, R.; Connelly, A.Connectivity-based fixel enhancement: Whole-brain statistical analysis of diffusion MRI measures in the presence of crossing fibres. Neuroimage, 2015, 15(117):40-55

* If using the -nonstationary option: Salimi-Khorshidi, G. Smith, S.M. Nichols, T.E. Adjusting the effect of nonstationarity in cluster-based and TFCE inference. NeuroImage, 2011, 54(3), 2006-19

--------------



**Author:** David Raffelt (david.raffelt@florey.edu.au) and Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2018 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/

MRtrix3 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/


