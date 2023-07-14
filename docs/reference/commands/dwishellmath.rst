.. _dwishellmath:

dwishellmath
============

Synopsis
--------

Apply an mrmath operation to each b-value shell in a DWI series

Usage
-----

::

    dwishellmath input operation output [ options ]

-  *input*: The input diffusion MRI series
-  *operation*: The operation to be applied to each shell; this must be one of the following: mean, median, sum, product, rms, norm, var, std, min, max, absmax, magmax
-  *output*: The output image series

Description
-----------

The output of this command is a 4D image, where each volume corresponds to a b-value shell (in order of increasing b-value), and the intensities within each volume correspond to the chosen statistic having been computed from across the DWI volumes belonging to that b-value shell.

Example usages
--------------

-   *To compute the mean diffusion-weighted signal in each b-value shell*::

        $ dwishellmath dwi.mif mean shellmeans.mif

Options
-------

Options for importing the diffusion gradient table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-grad** Provide the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Provide the diffusion gradient table in FSL bvecs/bvals format

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify the path in which to generate the scratch directory.

- **-continue <ScratchDir> <LastFile>** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

Standard options
^^^^^^^^^^^^^^^^

- **-info** display information messages.

- **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

- **-debug** display debugging messages.

- **-force** force overwrite of output files.

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

- **-config key value**  *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

- **-help** display this information page and exit.

- **-version** display version information and exit.

References
^^^^^^^^^^

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Daan Christiaens (daan.christiaens@kcl.ac.uk)

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

