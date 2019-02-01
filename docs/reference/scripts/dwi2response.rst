.. _dwi2response:

dwi2response
============

Synopsis
--------

Estimate response function(s) for spherical deconvolution

Usage
-----

::

    dwi2response algorithm [ options ] ...

-  *algorithm*: Select the algorithm to be used to complete the script operation; additional details and options become available once an algorithm is nominated. Options are: dhollander, fa, manual, msmt_5tt, tax, tournier

Description
-----------

dwi2response acts as a 'master' script for performing various types of response function estimation; a range of different algorithms are available for completing this task. When using this script, the name of the algorithm to be used must appear as the first argument on the command-line after 'dwi2response'. The subsequent compulsory arguments and options available depend on the particular algorithm being invoked.

Each algorithm available also has its own help page, including necessary references; e.g. to see the help page of the 'fa' algorithm, type 'dwi2response fa'.

Options
-------

Options common to all dwi2response algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-shells** The b-value shell(s) to use in response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-lmax** The maximum harmonic degree(s) of response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-mask** Provide an initial mask for response voxel selection

- **-voxels** Output an image showing the final voxel selection(s)

- **-grad** Pass the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Pass the diffusion gradient table in FSL bvecs/bvals format

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

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

- **-help** display this information page and exit.

- **-version** display version information and exit.

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au) and Thijs Dhollander (thijs.dhollander@gmail.com)

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

.. _dwi2response_dhollander:

dwi2response dhollander
=======================

Synopsis
--------

Use an improved version of the Dhollander et al. (2016) algorithm for unsupervised estimation of WM, GM and CSF response functions; does not require a T1 image (or segmentation thereof). This version of the Dhollander et al. (2016) algorithm was improved by Thijs Dhollander.

Usage
-----

::

    dwi2response dhollander input out_sfwm out_gm out_csf [ options ]

-  *input*: Input DWI dataset
-  *out_sfwm*: Output single-fibre WM response function text file
-  *out_gm*: Output GM response function text file
-  *out_csf*: Output CSF response function text file

Options
-------

Options specific to the 'dhollander' algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-erode** Number of erosion passes to apply to initial (whole brain) mask. Set to 0 to not erode the brain mask. (default: 3)

- **-fa** FA threshold for crude WM versus GM-CSF separation. (default: 0.2)

- **-sfwm** Final number of single-fibre WM voxels to select, as a percentage of refined WM. (default: 0.5 per cent)

- **-gm** Final number of GM voxels to select, as a percentage of refined GM. (default: 2 per cent)

- **-csf** Final number of CSF voxels to select, as a percentage of refined CSF. (default: 10 per cent)

Options common to all dwi2response algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-shells** The b-value shell(s) to use in response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-lmax** The maximum harmonic degree(s) of response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-mask** Provide an initial mask for response voxel selection

- **-voxels** Output an image showing the final voxel selection(s)

- **-grad** Pass the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Pass the diffusion gradient table in FSL bvecs/bvals format

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

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

- **-help** display this information page and exit.

- **-version** display version information and exit.

References
^^^^^^^^^^

* Dhollander, T.; Raffelt, D. & Connelly, A. Unsupervised 3-tissue response function estimation from single-shell or multi-shell diffusion MR data without a co-registered T1 image. ISMRM Workshop on Breaking the Barriers of Diffusion MRI, 2016, 5

* Dhollander, T.; Raffelt, D. & Connelly, A. Accuracy of response function estimation algorithms for 3-tissue spherical deconvolution of diverse quality diffusion MRI data. Proc Intl Soc Mag Reson Med, 2018, 26, 1569

--------------



**Author:** Thijs Dhollander (thijs.dhollander@gmail.com)

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

.. _dwi2response_fa:

dwi2response fa
===============

Synopsis
--------

Use the old FA-threshold heuristic for single-fibre voxel selection and response function estimation

Usage
-----

::

    dwi2response fa input output [ options ]

-  *input*: The input DWI
-  *output*: The output response function text file

Options
-------

Options specific to the 'fa' algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-erode** Number of brain mask erosion steps to apply prior to threshold (not used if mask is provided manually)

- **-number** The number of highest-FA voxels to use

- **-threshold** Apply a hard FA threshold, rather than selecting the top voxels

Options common to all dwi2response algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-shells** The b-value shell(s) to use in response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-lmax** The maximum harmonic degree(s) of response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-mask** Provide an initial mask for response voxel selection

- **-voxels** Output an image showing the final voxel selection(s)

- **-grad** Pass the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Pass the diffusion gradient table in FSL bvecs/bvals format

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

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

- **-help** display this information page and exit.

- **-version** display version information and exit.

References
^^^^^^^^^^

* Tournier, J.-D.; Calamante, F.; Gadian, D. G. & Connelly, A. Direct estimation of the fiber orientation density function from diffusion-weighted MRI data using spherical deconvolution. NeuroImage, 2004, 23, 1176-1185

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

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

.. _dwi2response_manual:

dwi2response manual
===================

Synopsis
--------

Derive a response function using an input mask image alone (i.e. pre-selected voxels)

Usage
-----

::

    dwi2response manual input in_voxels output [ options ]

-  *input*: The input DWI
-  *in_voxels*: Input voxel selection mask
-  *output*: Output response function text file

Options
-------

Options specific to the 'manual' algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-dirs** Manually provide the fibre direction in each voxel (a tensor fit will be used otherwise)

Options common to all dwi2response algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-shells** The b-value shell(s) to use in response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-lmax** The maximum harmonic degree(s) of response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-mask** Provide an initial mask for response voxel selection

- **-voxels** Output an image showing the final voxel selection(s)

- **-grad** Pass the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Pass the diffusion gradient table in FSL bvecs/bvals format

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

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

- **-help** display this information page and exit.

- **-version** display version information and exit.

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

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

.. _dwi2response_msmt_5tt:

dwi2response msmt_5tt
=====================

Synopsis
--------

Derive MSMT-CSD tissue response functions based on a co-registered five-tissue-type (5TT) image

Usage
-----

::

    dwi2response msmt_5tt input in_5tt out_wm out_gm out_csf [ options ]

-  *input*: The input DWI
-  *in_5tt*: Input co-registered 5TT image
-  *out_wm*: Output WM response text file
-  *out_gm*: Output GM response text file
-  *out_csf*: Output CSF response text file

Options
-------

Options specific to the 'msmt_5tt' algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-dirs** Manually provide the fibre direction in each voxel (a tensor fit will be used otherwise)

- **-fa** Upper fractional anisotropy threshold for GM and CSF voxel selection (default: 0.2)

- **-pvf** Partial volume fraction threshold for tissue voxel selection (default: 0.95)

- **-wm_algo algorithm** dwi2response algorithm to use for WM single-fibre voxel selection (default: tournier)

- **-sfwm_fa_threshold** Sets -wm_algo to fa and allows to specify a hard FA threshold for single-fibre WM voxels, which is passed to the -threshold option of the fa algorithm (warning: overrides -wm_algo option)

Options common to all dwi2response algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-shells** The b-value shell(s) to use in response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-lmax** The maximum harmonic degree(s) of response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-mask** Provide an initial mask for response voxel selection

- **-voxels** Output an image showing the final voxel selection(s)

- **-grad** Pass the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Pass the diffusion gradient table in FSL bvecs/bvals format

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

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

- **-help** display this information page and exit.

- **-version** display version information and exit.

References
^^^^^^^^^^

* Jeurissen, B.; Tournier, J.-D.; Dhollander, T.; Connelly, A. & Sijbers, J. Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data. NeuroImage, 2014, 103, 411-426

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

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

.. _dwi2response_tax:

dwi2response tax
================

Synopsis
--------

Use the Tax et al. (2014) recursive calibration algorithm for single-fibre voxel selection and response function estimation

Usage
-----

::

    dwi2response tax input output [ options ]

-  *input*: The input DWI
-  *output*: The output response function text file

Options
-------

Options specific to the 'tax' algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-peak_ratio** Second-to-first-peak amplitude ratio threshold

- **-max_iters** Maximum number of iterations

- **-convergence** Percentile change in any RF coefficient required to continue iterating

Options common to all dwi2response algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-shells** The b-value shell(s) to use in response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-lmax** The maximum harmonic degree(s) of response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-mask** Provide an initial mask for response voxel selection

- **-voxels** Output an image showing the final voxel selection(s)

- **-grad** Pass the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Pass the diffusion gradient table in FSL bvecs/bvals format

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

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

- **-help** display this information page and exit.

- **-version** display version information and exit.

References
^^^^^^^^^^

* Tax, C. M.; Jeurissen, B.; Vos, S. B.; Viergever, M. A. & Leemans, A. Recursive calibration of the fiber response function for spherical deconvolution of diffusion MRI data. NeuroImage, 2014, 86, 67-80

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

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

.. _dwi2response_tournier:

dwi2response tournier
=====================

Synopsis
--------

Use the Tournier et al. (2013) iterative algorithm for single-fibre voxel selection and response function estimation

Usage
-----

::

    dwi2response tournier input output [ options ]

-  *input*: The input DWI
-  *output*: The output response function text file

Options
-------

Options specific to the 'tournier' algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-iter_voxels** Number of single-fibre voxels to select when preparing for the next iteration

- **-sf_voxels** Number of single-fibre voxels to use when calculating response function

- **-dilate** Number of mask dilation steps to apply when deriving voxel mask to test in the next iteration

- **-max_iters** Maximum number of iterations

Options common to all dwi2response algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-shells** The b-value shell(s) to use in response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-lmax** The maximum harmonic degree(s) of response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-mask** Provide an initial mask for response voxel selection

- **-voxels** Output an image showing the final voxel selection(s)

- **-grad** Pass the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Pass the diffusion gradient table in FSL bvecs/bvals format

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

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

- **-help** display this information page and exit.

- **-version** display version information and exit.

References
^^^^^^^^^^

* Tournier, J.-D.; Calamante, F. & Connelly, A. Determination of the appropriate b-value and number of gradient directions for high-angular-resolution diffusion-weighted imaging. NMR Biomedicine, 2013, 26, 1775-1786

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

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

