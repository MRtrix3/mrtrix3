.. _dwibiascorrect:

dwibiascorrect
==============

Synopsis
--------

Perform B1 field inhomogeneity correction for a DWI volume series

Usage
-----

::

    dwibiascorrect algorithm [ options ] ...

-  *algorithm*: Select the algorithm to be used to complete the script operation; additional details and options become available once an algorithm is nominated. Options are: ants, fsl

Options
-------

Options for importing the diffusion gradient table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-grad** Provide the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Provide the diffusion gradient table in FSL bvecs/bvals format

Options common to all dwibiascorrect algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-mask image** Manually provide a mask image for bias field estimation

- **-bias image** Output the estimated bias field

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

.. _dwibiascorrect_ants:

dwibiascorrect ants
===================

Synopsis
--------

Perform DWI bias field correction using the N4 algorithm as provided in ANTs

Usage
-----

::

    dwibiascorrect ants input output [ options ]

-  *input*: The input image series to be corrected
-  *output*: The output corrected image series

Options
-------

Options for ANTs N4BiasFieldCorrection command
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-ants.b [100,3]** N4BiasFieldCorrection option -b. [initial mesh resolution in mm, spline order] This value is optimised for human adult data and needs to be adjusted for rodent data.

- **-ants.c [1000,0.0]** N4BiasFieldCorrection option -c. [numberOfIterations,convergenceThreshold]

- **-ants.s 4** N4BiasFieldCorrection option -s. shrink-factor applied to spatial dimensions

Options for importing the diffusion gradient table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-grad** Provide the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Provide the diffusion gradient table in FSL bvecs/bvals format

Options common to all dwibiascorrect algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-mask image** Manually provide a mask image for bias field estimation

- **-bias image** Output the estimated bias field

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

* Tustison, N.; Avants, B.; Cook, P.; Zheng, Y.; Egan, A.; Yushkevich, P. & Gee, J. N4ITK: Improved N3 Bias Correction. IEEE Transactions on Medical Imaging, 2010, 29, 1310-1320

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

.. _dwibiascorrect_fsl:

dwibiascorrect fsl
==================

Synopsis
--------

Perform DWI bias field correction using the 'fast' command as provided in FSL

Usage
-----

::

    dwibiascorrect fsl input output [ options ]

-  *input*: The input image series to be corrected
-  *output*: The output corrected image series

Description
-----------

The FSL 'fast' command only estimates the bias field within a brain mask, and cannot extrapolate this smoothly-varying field beyond the defined mask. As such, this algorithm by necessity introduces a hard masking of the input DWI. Since this attribute may interfere with the purpose of using the command (e.g. correction of a bias field is commonly used to improve brain mask estimation), use of this particular algorithm is generally not recommended.

Options
-------

Options for importing the diffusion gradient table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-grad** Provide the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Provide the diffusion gradient table in FSL bvecs/bvals format

Options common to all dwibiascorrect algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-mask image** Manually provide a mask image for bias field estimation

- **-bias image** Output the estimated bias field

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

* Zhang, Y.; Brady, M. & Smith, S. Segmentation of brain MR images through a hidden Markov random field model and the expectation-maximization algorithm. IEEE Transactions on Medical Imaging, 2001, 20, 45-57

* Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.; Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.; Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.; Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. Advances in functional and structural MR image analysis and implementation as FSL. NeuroImage, 2004, 23, S208-S219

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

