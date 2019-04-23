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

-  *input output*: list of all input and output tissue compartment files (see example usage).

Description
-----------

This command inputs any number of tissue components (e.g. from multi-tissue CSD) and outputs corresponding normalised tissue components. Intensity normalisation is performed in the log-domain, and can smoothly vary spatially to accomodate the effects of (residual) intensity inhomogeneities.

The -mask option is mandatory and is optimally provided with a brain mask (such as the one obtained from dwi2mask earlier in the processing pipeline). Outlier areas with exceptionally low or high combined tissue contributions are accounted for and reoptimised as the intensity inhomogeneity estimation becomes more accurate.

Example usages
--------------

-   *Default usage (for 3-tissue CSD compartments)*::

        $ mtnormalise wmfod.mif wmfod_norm.mif gm.mif gm_norm.mif csf.mif csf_norm.mif -mask mask.mif

    Note how for each tissue compartment, the input and output images are provided as a consecutive pair.

Options
-------

-  **-mask image** the mask defines the data used to compute the intensity normalisation. This option is mandatory.

-  **-order number** the maximum order of the polynomial basis used to fit the normalisation field in the log-domain. An order of 0 is equivalent to not allowing spatial variance of the intensity normalisation factor. (default: 3)

-  **-niter number** set the number of iterations. (default: 15)

-  **-check_norm image** output the final estimated spatially varying intensity level that is used for normalisation.

-  **-check_mask image** output the final mask used to compute the normalisation. This mask excludes regions identified as outliers by the optimisation process.

-  **-value number** specify the (positive) reference value to which the summed tissue compartments will be normalised. (default: 0.282095, SH DC term for unit angular integral)

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------



**Author:** Thijs Dhollander (thijs.dhollander@gmail.com), Rami Tabbara (rami.tabbara@florey.edu.au) and David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (c) 2008-2019 the MRtrix3 contributors.

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


