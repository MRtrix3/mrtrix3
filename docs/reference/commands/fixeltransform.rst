.. _fixeltransform:

fixeltransform
===================

Synopsis
--------

Transform a fixel dataset

Usage
--------

::

    fixeltransform [ options ]  fixel_in warp fixel_out

-  *fixel_in*: the input fixel directory
-  *warp*: the 4D deformation field
-  *fixel_out*: the output fixel directory

Description
-----------

Unlike the fixelreorient command, which does not move fixels in space but just reorients them in place based on the premise of a prior transformation having been applied, this command additionally involves applying a spatial transformation to input fixel data.

Because it is not trivial to interpolate fixel data at sub-voxel locations, the resampling following transformation is performed using nearest-neighbour interpolation. This also means that there may be some fixels in the input dataset for which there is no corresponding fixel created in the output dataset, as well as fixels in the input dataset for which there are multiple corresponding fixels created in the output dataset. Finally, there is no assurance of any form of fixel correspondence between the input and output datasets.

The output fixel dataset will consist of the compulsory index and directions images, and resampled versions of any fixel data files found in the input directory. Any voxel images present in the input fixel directory will be skipped. Fixel data files with more than one column are currently not supported. This command does not apply any modulation to fixel-wise data based on the deformation applied.

Options
-------

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

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

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


