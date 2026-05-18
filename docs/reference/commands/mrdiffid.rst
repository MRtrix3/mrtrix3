.. _mrdiffid:

mrdiffid
===================

Synopsis
--------

Compute the Differential Identifiability measure across an image dataset

Usage
--------

::

    mrdiffid [ options ]  images ids

-  *images*: a text file containing the filesystem paths of the images to be processed (one path per line);
-  *ids*: a text file containing a 1D integer matrix of subject identifiers, with one entry per input image

Description
-----------

Given a set of input images and a corresponding list of integer subject identifiers, this command computes pairwise cosine similarities between all subjects, and derives the Differential Identifiability measure I_diff.

I_diff = (I_self - I_others) x 100, where I_self is the mean pairwise cosine similarity for subjects sharing the same identifier, and I_others is the mean pairwise cosine similarity for subjects with differing identifiers.

This command operates equivalently on volumetric images and fixel data files. All input images must possess identical dimensions; volumetric images must additionally reside on a common voxel grid in scanner space, their intensity data being serialised into the columns of the data matrix.

The -mask option modulates which image elements contribute to the quantification. For volumetric inputs the mask is a 3D image, every non-zero voxel of which contributes one row to the data matrix; for fixel data files the mask is itself a fixel data file, selecting which fixels are carried through to the data matrix.

Options
-------

-  **-mask image** only include those image elements within the specified mask in the computation of Differential Identifiability; a volumetric image for volumetric inputs, or a fixel data file for fixel inputs

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status; alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages & debug input data.

-  **-force** force overwrite of output files (caution: using the same file as input and output might cause unexpected behaviour).

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-config key value** *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

Amico E, Goni J. The quest for identifiability in human functional connectomes. Scientific Reports, 2018, 8(1), 8254.

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2026 the MRtrix3 contributors.

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


