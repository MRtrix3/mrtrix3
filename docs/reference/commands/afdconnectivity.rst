.. _afdconnectivity:

afdconnectivity
===================

Synopsis
--------

Obtain an estimate of fibre connectivity between two regions using AFD and streamlines tractography

Usage
--------

::

    afdconnectivity [ options ]  image tracks

-  *image*: the input FOD image.
-  *tracks*: the input track file defining the bundle of interest.

Description
-----------

This estimate is obtained by determining a fibre volume (AFD) occupied by the pathway of interest, and dividing by the streamline length.

If only the streamlines belonging to the pathway of interest are provided, then ALL of the fibre volume within each fixel selected will contribute to the result. If the -wbft option is used to provide whole-brain fibre-tracking (of which the pathway of interest should contain a subset), only the fraction of the fibre volume in each fixel estimated to belong to the pathway of interest will contribute to the result.

Use -quiet to suppress progress messages and output fibre connectivity value only.

For valid comparisons of AFD connectivity across scans, images MUST be intensity normalised and bias field corrected, and a common response function for all subjects must be used.

Note that the sum of the AFD is normalised by streamline length to account for subject differences in fibre bundle length. This normalisation results in a measure that is more related to the cross-sectional volume of the tract (and therefore 'connectivity'). Note that SIFT-ed tract count is a superior measure because it is unaffected by tangential yet unrelated fibres. However, AFD connectivity may be used as a substitute when Anatomically Constrained Tractography is not possible due to uncorrectable EPI distortions, and SIFT may therefore not be as effective.

Longer discussion regarding this command can additionally be found at: https://mrtrix.readthedocs.io/en/3.0.7/concepts/afd_connectivity.html (as well as in the relevant reference).

Options
-------

-  **-wbft tracks** provide a whole-brain fibre-tracking data set (of which the input track file should be a subset), to improve the estimate of fibre bundle volume in the presence of partial volume

-  **-afd_map image** output a 3D image containing the AFD estimated for each voxel.

-  **-all_fixels** if whole-brain fibre-tracking is NOT provided, then if multiple fixels within a voxel are traversed by the pathway of interest, by default the fixel with the greatest streamlines density is selected to contribute to the AFD in that voxel. If this option is provided, then ALL fixels with non-zero streamlines density will contribute to the result, even if multiple fixels per voxel are selected.

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

Smith, R. E.; Raffelt, D.; Tournier, J.-D.; Connelly, A. Quantitative Streamlines Tractography: Methods and Inter-Subject Normalisation. Open Science Framework, https://doi.org/10.31219/osf.io/c67kn.

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** David Raffelt (david.raffelt@florey.edu.au) and Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2025 the MRtrix3 contributors.

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


