.. _dwibiascorrect:

dwibiascorrect
==============

Synopsis
--------

Perform B1 field inhomogeneity correction for a DWI volume series

Usage
--------

::

    dwibiascorrect input output [ options ]

-  *input*: The input image series to be corrected
-  *output*: The output corrected image series

Options
-------

- **-mask image** Manually provide a mask image for bias field estimation

- **-bias image** Output the estimated bias field

Options for selection of bias field estimation software (one of these MUST be provided)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-ants** Use ANTS N4 to estimate the inhomogeneity field

- **-fsl** Use FSL FAST to estimate the inhomogeneity field

Options for providing the DWI gradient table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-grad** Pass the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Pass the diffusion gradient table in FSL bvecs/bvals format

Options for ANTS N4BiasFieldCorrection
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-ants.b [100,3]** N4BiasFieldCorrection option -b. [initial mesh resolution in mm, spline order] This value is optimised for human adult data and needs to be adjusted for rodent data.

- **-ants.c [1000,0.0]** N4BiasFieldCorrection option -c. [numberOfIterations,convergenceThreshold]

- **-ants.s 4** N4BiasFieldCorrection option -s. shrink-factor applied to spatial dimensions

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete temporary files during script execution, and do not delete temporary directory at script completion

- **-tempdir /path/to/tmp/** manually specify the path in which to generate the temporary directory

- **-continue <TempDir> <LastFile>** continue the script from a previous execution; must provide the temporary directory path, and the name of the last successfully-generated file

Standard options
^^^^^^^^^^^^^^^^

- **-info** display information messages.

- **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

- **-debug** display debugging messages.

- **-force** force overwrite of output files.

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

- **-help** display this information page and exit.

- **-version** display version information and exit.

References
^^^^^^^^^^

* If using -fast option: Zhang, Y.; Brady, M. & Smith, S. Segmentation of brain MR images through a hidden Markov random field model and the expectation-maximization algorithm. IEEE Transactions on Medical Imaging, 2001, 20, 45-57

* If using -fast option: Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.; Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.; Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.; Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. Advances in functional and structural MR image analysis and implementation as FSL. NeuroImage, 2004, 23, S208-S219

* If using -ants option: Tustison, N.; Avants, B.; Cook, P.; Zheng, Y.; Egan, A.; Yushkevich, P. & Gee, J. N4ITK: Improved N3 Bias Correction. IEEE Transactions on Medical Imaging, 2010, 29, 1310-1320

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2018 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/

MRtrix3 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/

