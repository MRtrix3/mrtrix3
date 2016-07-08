.. _mrclusterstats:

mrclusterstats
===========

Synopsis
--------

::

    mrclusterstats [ options ]  input design contrast mask output

-  *input*: a text file containing the file names of the input images, one file per line
-  *design*: the design matrix, rows should correspond with images in the input image text file
-  *contrast*: the contrast matrix, only specify one contrast as it will automatically compute the opposite contrast.
-  *mask*: a mask used to define voxels included in the analysis.
-  *output*: the filename prefix for all output.

Description
-----------

Voxel-based analysis using permutation testing and threshold-free cluster enhancement.

Options
-------

-  **-negative** automatically test the negative (opposite) contrast. By computing the opposite contrast simultaneously the computation time is reduced.

-  **-nperms num** the number of permutations (default = 5000).

-  **-threshold value** the cluster-forming threshold to use for a standard cluster-based analysis. This disables TFCE, which is the default otherwise.

-  **-tfce_dh value** the height increment used in the TFCE integration (default = 0.1)

-  **-tfce_e value** TFCE extent parameter (default = 0.5)

-  **-tfce_h value** TFCE height parameter (default = 2)

-  **-connectivity** use 26-voxel-neighbourhood connectivity (Default: 6)

-  **-nonstationary** perform non-stationarity correction (currently only implemented with tfce)

-  **-nperms_nonstationary num** the number of permutations used when precomputing the empirical statistic image for nonstationary correction (Default: 5000)

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

* If not using the -threshold command-line option:Smith, S. M. & Nichols, T. E. Threshold-free cluster enhancement: Addressing problems of smoothing, threshold dependence and localisation in cluster inference. NeuroImage, 2009, 44, 83-98

* If using the -nonstationary option:Salimi-Khorshidi, G. Smith, S.M. Nichols, T.E. Adjusting the effect of nonstationarity in cluster-based and TFCE inference. Neuroimage, 2011, 54(3), 2006-19

--------------



**Author:** David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

