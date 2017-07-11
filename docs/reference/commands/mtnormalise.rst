.. _mtnormalise:

mtnormalise
===================

Synopsis
--------

Multi-tissue informed log-domain intensity normalisation

Usage
--------

::

    mtnormalise [ options ]  input output [ input output ... ]

-  *input output*: list of all input and output tissue compartment files. See example usage in the description.

Description
-----------

This command inputs any number of tissue components (e.g. from multi-tissue CSD) and outputs corresponding normalised tissue components. Intensity normalisation is performed in the log-domain, and can smoothly vary spatially to accomodate the effects of (residual) intensity inhomogeneities.

The -mask option is mandatory and is optimally provided with a brain mask (such as the one obtained from dwi2mask earlier in the processing pipeline). Outlier areas with exceptionally low or high combined tissue contributions are accounted for and reoptimised as the intensity inhomogeneity estimation becomes more accurate.

Example usage: mtnormalise wmfod.mif wmfod_norm.mif gm.mif gm_norm.mif csf.mif csf_norm.mif -mask mask.mif.

Options
-------

-  **-mask image** the mask defines the data used to compute the intensity normalisation. This option is mandatory.

-  **-niter number** set the number of iterations. (default: 15)

-  **-check_norm image** output the final estimated spatially varying intensity level that is used for normalisation.

-  **-check_mask image** output the final mask used to compute the normalisation. This mask excludes regions identified as outliers by the optimisation process.

-  **-value number** specify the reference (positive) value to which the summed tissue compartments will be normalised. (default: 0.282095, SH DC term for unit angular integral)

-  **-poly_order order** the order of the polynomial function used to fit the normalisation field. (default: 3)

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

--------------



**Author:** Thijs Dhollander (thijs.dhollander@gmail.com), Rami Tabbara (rami.tabbara@florey.edu.au) and David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


