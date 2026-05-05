.. _fixel2fixel:

fixel2fixel
===================

Synopsis
--------

Project a fixel-wise quantity from one fixel dataset to another

Usage
--------

::

    fixel2fixel [ options ]  data_in correspondence metric directory_out data_out

-  *data_in*: the source fixel data file
-  *correspondence*: the .npz file containing the fixel-fixel correspondence mapping
-  *metric*: the metric to calculate when mapping multiple input fixels to an output fixel; options are: sum, mean, count, angle
-  *directory_out*: the output fixel directory in which the output fixel data file will be placed
-  *data_out*: the name of the output fixel data file

Description
-----------

This command requires pre-calculation of fixel correspondence between two fixel datasets; this would most typically be achieved using the fixelcorrespondence command, which produces a .npz file as input to this command.

The -weighted option does not act as a per-fixel value multipler as is done in the calculation of the Fibre Density and Cross-section (FDC) measure. Rather, whenever a quantitative value for a target fixel is to be determined from the aggregation of multiple source fixels, the fixel data file provided via the -weights option will be used to modulate the magnitude by which each source fixel contributes to that aggregate. Most typically this would be a file containing fixel densities / volumes, if e.g. the value for a low-density source fixel should not contribute as much as a high-density source fixel in calculation of a weighted mean value for a target fixel.

Example usages
--------------

-   *Project a fixel-wise additive measure (such as fibre density)*::

        $ fixel2fixel subject/fd.mif fixelmapping.npz sum template subject_fd.mif

    For a measure that is naturally additive, such as is the case for a fibre density measure relating to axonal volume, the 'sum' metric should be used; ie. if two subject fixels map to a single template fixel, then the fibre density ascribed to that template fixel should be the sum of the fibre densities of the two subject fixels.

-   *Project a fixel-wise non-additive measure (such as axonal diameter)*::

        $ fixel2fixel subject/ad.mif fixelmapping.npz mean template subject_ad.mif -weighted subject/fd.mif

    For some fixel-wise measures, such as axonal diameter in this example, it would not be suitable to sum those measures across multiple fixels. Eg. if two subject fixels, with ascribed axonal diameters of 2um and 4um, needed to be merged in order to map to a single template fixel, then it would not be suitable to assign an axonal diameter of 6um to the template fixel; in the absence of any other information, a value of 3um would intuitively be more suitable. This can be further improved by specifying the -weighted option, providing as input a fixel data file encoding some form of fibre density: if eg. the 4um fixel is three times more dense than the 2um fixel, then the weighted mean value projected to the template fixel should be 3.5um.

-   *Replicate the behaviour of the fixelcorrespondence command from MRtrix version 3.0.x*::

        $ fixelcorrespondence subject/fd.mif template/fd.mif fixelmapping.npz -algorithm legacy; fixel2fixel subject/fd.mif fixelmapping.npz sum fd_template subject.mif -ignore_weights

    To reproduce the behaviour of the 3.0.x version of the fixelcorrespondence command requires two explicit modifications to the default behaviours of both the new fixelcorrespondence command and command fixel2fixel. When running the new fixelcorrespondence command, matching algorithm 'legacy' must be used; this simply chooses for each template fixel the nearest subject fixel, provided that it is within some maximal angular distance. However if multiple template fixels were to select the same subject fixel, the entirety of the fibre density in that fixel would be projected to both template fixels. Under the default behaviour of command fixel2fixel, the fibre density of that subject fixel would instead be split between those two template fixels. Option -ignore_weights disables that behaviour, thereby manifesting the same (potentially undesirable) behaviour of the earlier software. Note that this example is provided for understanding and backwards compatibility and should not be interpreted as explicit advocacy for its use.

Options
-------

-  **-weighted weights_in** specify fixel data file containing weights to use during aggregation of multiple source fixels

-  **-ignore_weights** do not apply the fixel-fixel mapping weights as stored in the correspondence data file

Options relating to filling data values for specific fixels
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-fill value** value for output fixels to which no input fixels are mapped (default: 0)

-  **-nan_many2one** insert NaN value in cases where multiple input fixels map to the same output fixel

-  **-nan_one2many** insert NaN value in cases where one input fixel maps to multiple output fixels

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


