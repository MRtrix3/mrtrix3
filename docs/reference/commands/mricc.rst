.. _mricc:

mricc
===================

Synopsis
--------

Compute the Intraclass Correlation Coefficient (ICC) across an image dataset

Usage
--------

::

    mricc [ options ]  images form design output

-  *images*: a text file containing the filesystem paths of the images to be processed (one path per line)
-  *form*: the form of Intraclass Correlation Coefficient to compute (one of: icc_1_1, icc_2_1, icc_3_1, icc_1_k, icc_2_k, icc_3_k)
-  *design*: a text file with one row per input image, containing either subject identifiers only (one-way models) or subject and measurement identifiers (two-way models)
-  *output*: the output image of per-element ICC values

Description
-----------

Given a set of input images and a corresponding design file, this command computes, for each image element independently, a chosen form of the Intraclass Correlation Coefficient (ICC).

This command operates equivalently on volumetric images and fixel data files; all input images must possess identical dimensions (and, for volumetric images, identical voxel grids in scanner space), and the computation is performed independently for every image element.

The available forms of ICC are labelled using the (model, form) notation of Shrout & Fleiss (1979), where the second index "k" denotes the average-measurement variants: "icc_1_1" is one-way random effects, single measurement; "icc_2_1" is two-way random effects, absolute agreement, single measurement; "icc_3_1" is two-way mixed effects, consistency, single measurement; "icc_1_k", "icc_2_k" and "icc_3_k" are the corresponding average-measurement variants.

The design file is a text file with one row per input image, its columns being delimiter-separated (whitespace, comma or semicolon may be used as the delimiter). The number of columns required, and their interpretation, depends on the model of the chosen ICC form.

For the one-way random effects models ("icc_1_1" and "icc_1_k"), the design file must contain a single column: a subject identifier for each input image. Each subject must be represented by an equal number of images; no correspondence of measurements across subjects is modelled, and the ordering of images within a subject is immaterial.

For the two-way models ("icc_2_1", "icc_3_1", "icc_2_k" and "icc_3_k"), the design file must contain two columns: a subject identifier followed by a measurement identifier. A balanced design is required: every subject must possess exactly one image for every measurement identifier, and the same set of measurement identifiers must be present for all subjects. Because each input image is explicitly tagged with both a subject identifier and a measurement identifier, a consistent ordering of measurements across subjects is not assumed.

Options
-------

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

Shrout PE, Fleiss JL. Intraclass correlations: uses in assessing rater reliability. Psychological Bulletin, 1979, 86(2), 420-428.

McGraw KO, Wong SP. Forming inferences about some intraclass correlation coefficients. Psychological Methods, 1996, 1(1), 30-46.

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


