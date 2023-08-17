.. _fixelcorrespondence:

fixelcorrespondence
===================

Synopsis
--------

Establish correpondence between two fixel datasets

Usage
--------

::

    fixelcorrespondence [ options ]  source_density target_density output

-  *source_density*: the input source fixel data file corresponding to the FD or FDC metric
-  *target_density*: the input target fixel data file corresponding to the FD or FDC metric
-  *output*: the name of the output directory encoding the fixel correspondence

Description
-----------

It is assumed that the source image has already been spatially normalised and is defined on the same voxel grid as the target. One would typically also want to have performed a reorientation of fibre information to reflect this spatial normalisation prior to invoking this command, as this would be expected to improve fibre orientation correspondence across datasets.

The output of the command is a directory encoding how data from source fixels should be remapped in order to express those data in target fixel space. This information would typically then be utilised by command fixel2fixel to project some quantitative parameter from the source fixel dataset to the target fixels.

Multiple algorithms are provided; a brief description of each of these is provided below.

"all2all": This algorithm is defined for debugging / demonstrative purposes only. It assigns all source fixels to all target fixels, and is therefore not appropriate for practical use.

"nearest": This algorithm duplicates the behaviour of the fixelcorrespondence command in MRtrix versions 3.0.x. and earlier. It determines, for every target fixel, the nearest source fixel, and then assigns that source fixel to the target fixel as long as the angle between them is less than some threshold.

"ismrm2018": This is a combinatorial algorithm, for which the algorithm and cost function are described in the relevant reference (Smith et al., 2018).

"in2023": This is a combinatorial algorithm, for which the combinatorial algorithm utilised is described in reference (Smith et al., 2018), but an alternative cost function is proposed (publication pending).

Options
-------

-  **-algorithm choice** the algorithm to use when establishing fixel correspondence; options are: all2all,nearest,ismrm2018,in2023 (default: in2023)

-  **-remapped path** export the remapped source fixels to a new fixel directory

Options specific to algorithm "nearest"
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-angle value** maximum angle within which a corresponding fixel may be selected, in degrees (default: 45)

Options specific to algorithm "in2023"
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-constants alpha beta** set values for the two constants that modulate the influence of different cost function terms

Options applicable to all combinatorial-based algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-max_origins value** maximal number of origin source fixels for an individual target fixel (default: 3)

-  **-max_objectives value** maximal number of objective target fixels for an individual source fixel (default: 3)

-  **-cost path** export a 3D image containing the optimal value of the relevant cost function in each voxel

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

* If using -algorithm ismrm2018 or -algorithm in2023: Smith, R.E.; Connelly, A. Mitigating the effects of imperfect fixel correspondence in Fixel-Based Analysis. In Proc ISMRM 2018: 456.

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


