.. _fixel2voxel:

fixel2voxel
===================

Synopsis
--------

Convert a fixel-based sparse-data image into some form of scalar image

Usage
--------

::

    fixel2voxel [ options ]  fixel_in operation image_out

-  *fixel_in*: the input fixel data file
-  *operation*: the operation to apply, one of: mean, sum, product, min, max, absmax, magmax, count, complexity, sf, dec_unit, dec_scaled, none.
-  *image_out*: the output scalar image.

Description
-----------

Fixel data can be reduced to voxel data in a number of ways:

- Some statistic computed across all fixel values within a voxel: mean, sum, product, min, max, absmax, magmax

- The number of fixels in each voxel: count

- Some measure of crossing-fibre organisation: complexity, sf ('single-fibre')

- A 4D directionally-encoded colour image: dec_unit, dec_scaled

- A 4D image containing all fixel data values in each voxel unmodified: none

The -weighted option deals with the case where there is some per-fixel metric of interest that you wish to collapse into a single scalar measure per voxel, but each fixel possesses a different volume, and you wish for those fixels with greater volume to have a greater influence on the calculation than fixels with lesser volume. For instance, when estimating a voxel-based measure of mean axon diameter from per-fixel mean axon diameters, a fixel's mean axon diameter should be weigthed by its relative volume within the voxel in the calculation of that voxel mean.

Options
-------

-  **-number N** use only the largest N fixels in calculation of the voxel-wise statistic; in the case of operation "none", output only the largest N fixels in each voxel.

-  **-fill value** for "none" operation, specify the value to fill when number of fixels is fewer than the maximum (default: 0.0)

-  **-weighted fixel_in** weight the contribution of each fixel to the per-voxel result according to its volume.

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

* Reference for 'complexity' operation: |br|
  Riffert, T. W.; Schreiber, J.; Anwander, A. & Knosche, T. R. Beyond Fractional Anisotropy: Extraction of bundle-specific structural metrics from crossing fibre models. NeuroImage, 2014, 100, 176-191

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au) & David Raffelt (david.raffelt@florey.edu.au)

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


