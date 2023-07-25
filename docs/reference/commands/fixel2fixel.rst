.. _fixel2fixel:

fixel2fixel
===================

Synopsis
--------

Project quantities from one fixel dataset to another

Usage
--------

::

    fixel2fixel [ options ]  data_in correspondence metric directory_out data_out

-  *data_in*: the source fixel data file
-  *correspondence*: the directory containing the fixel-fixel correspondence mapping
-  *metric*: the metric to calculate when mapping multiple input fixels to an output fixel; options are: sum, mean, count, angle
-  *directory_out*: the output fixel directory in which the output fixel data file will be placed
-  *data_out*: the name of the output fixel data file

Description
-----------

This command requires pre-calculation of fixel correspondence between two fixel datasets; this would most typically be achieved using the fixelcorrespondence command.

The -weighted option does not act as a per-fixel value multipler as is done in the calculation of the Fibre Density and Cross-section (FDC) measure. Rather, whenever a quantitative value for a target fixel is to be determined from the aggregation of multiple source fixels, the fixel data file provided via the -weights option will be used to modulate the magnitude by which each source fixel contributes to that aggregate. Most typically this would be a file containing fixel densities / volumes, if e.g. the value for a low-density source fixel should not contribute as much as a high-density source fixel in calculation of a weighted mean value for a target fixel.

Options
-------

-  **-weighted weights_in** specify fixel data file containing weights to use during aggregation of multiple source fixels

Options relating to filling data values for specific fixels
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-fill value** value for output fixels to which no input fixels are mapped (default: 0)

-  **-nan_many2one** insert NaN value in cases where multiple input fixels map to the same output fixel

-  **-nan_one2many** insert NaN value in cases where one input fixel maps to multiple output fixels

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


