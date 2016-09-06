.. _dwi2response_dhollander:

dwi2response dhollander
=======================

Synopsis
--------

::

    dwi2response dhollander [ options ] input out_sfwm out_gm out_csf

-  *input*: The input DWI
-  *out_sfwm*: Output single-fibre WM response text file
-  *out_gm*: Output GM response text file
-  *out_csf*: Output CSF response text file

Description
-----------

Unsupervised estimation of WM, GM and CSF response functions. Does not require a T1 image (or segmentation thereof).

Options
-------

Options specific to the 'dhollander' algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-erode** Number of erosion passes to apply to initial (whole brain) mask. (default: 3)

- **-fa** FA threshold for crude WM versus GM-CSF separation. (default: 0.2)

- **-sfwm** Number of single-fibre WM voxels to select, as a percentage of refined WM. (default: 0.5 per cent)

- **-gm** Number of GM voxels to select, as a percentage of refined GM. (default: 2 per cent)

- **-csf** Number of CSF voxels to select, as a percentage of refined CSF. (default: 10 per cent)

Options common to all dwi2response algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-shell** The b-value shell(s) to use in response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-lmax** The maximum harmonic degree(s) of response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-mask** Provide an initial mask for response voxel selection

- **-voxels** Output an image showing the final voxel selection(s)

- **-grad** Pass the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Pass the diffusion gradient table in FSL bvecs/bvals format

Standard options
^^^^^^^^^^^^^^^^

- **-continue <TempDir> <LastFile>** Continue the script from a previous execution; must provide the temporary directory path, and the name of the last successfully-generated file

- **-force** Force overwrite of output files if pre-existing

- **-help** Display help information for the script

- **-nocleanup** Do not delete temporary files during script, or temporary directory at script completion

- **-nthreads number** Use this number of threads in MRtrix multi-threaded applications (0 disables multi-threading)

- **-tempdir /path/to/tmp/** Manually specify the path in which to generate the temporary directory

- **-quiet** Suppress all console output during script execution

- **-verbose** Display additional information for every command invoked

References
^^^^^^^^^^

* If using 'dhollander' algorithm: Dhollander, T.; Raffelt, D. & Connelly, A. Unsupervised 3-tissue response function estimation from single-shell or multi-shell diffusion MR data without a co-registered T1 image. ISMRM Workshop on Breaking the Barriers of Diffusion MRI, 2016, 5

* If using 'fa' algorithm: Tournier, J.-D.; Calamante, F.; Gadian, D. G. & Connelly, A. Direct estimation of the fiber orientation density function from diffusion-weighted MRI data using spherical deconvolution. NeuroImage, 2004, 23, 1176-1185

* If using 'msmt_csd' algorithm: Jeurissen, B.; Tournier, J.-D.; Dhollander, T.; Connelly, A. & Sijbers, J. Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data. NeuroImage, 2014, 103, 411-426

* If using 'tax' algorithm: Tax, C. M.; Jeurissen, B.; Vos, S. B.; Viergever, M. A. & Leemans, A. Recursive calibration of the fiber response function for spherical deconvolution of diffusion MRI data. NeuroImage, 2014, 86, 67-80

* If using 'tournier' algorithm: Tournier, J.-D.; Calamante, F. & Connelly, A. Determination of the appropriate b value and number of gradient directions for high-angular-resolution diffusion-weighted imaging. NMR Biomedicine, 2013, 26, 1775-1786

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au) and Thijs Dhollander (thijs.dhollander@gmail.com)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

.. _dwi2response_fa:

dwi2response fa
===============

Synopsis
--------

::

    dwi2response fa [ options ] input output

-  *input*: The input DWI
-  *output*: The output response function text file

Description
-----------

Use the old FA-threshold heuristic for single-fibre voxel selection and response function estimation

Options
-------

Options specific to the 'fa' algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-erode** Number of brain mask erosion steps to apply prior to threshold (not used if mask is provided manually)

- **-number** The number of highest-FA voxels to use

- **-threshold** Apply a hard FA threshold, rather than selecting the top voxels

Options common to all dwi2response algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-shell** The b-value shell(s) to use in response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-lmax** The maximum harmonic degree(s) of response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-mask** Provide an initial mask for response voxel selection

- **-voxels** Output an image showing the final voxel selection(s)

- **-grad** Pass the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Pass the diffusion gradient table in FSL bvecs/bvals format

Standard options
^^^^^^^^^^^^^^^^

- **-continue <TempDir> <LastFile>** Continue the script from a previous execution; must provide the temporary directory path, and the name of the last successfully-generated file

- **-force** Force overwrite of output files if pre-existing

- **-help** Display help information for the script

- **-nocleanup** Do not delete temporary files during script, or temporary directory at script completion

- **-nthreads number** Use this number of threads in MRtrix multi-threaded applications (0 disables multi-threading)

- **-tempdir /path/to/tmp/** Manually specify the path in which to generate the temporary directory

- **-quiet** Suppress all console output during script execution

- **-verbose** Display additional information and progress for every command invoked

- **-debug** Display additional debugging information over and above the verbose output

References
^^^^^^^^^^

* If using 'dhollander' algorithm: Dhollander, T.; Raffelt, D. & Connelly, A. Unsupervised 3-tissue response function estimation from single-shell or multi-shell diffusion MR data without a co-registered T1 image. ISMRM Workshop on Breaking the Barriers of Diffusion MRI, 2016, 5

* If using 'fa' algorithm: Tournier, J.-D.; Calamante, F.; Gadian, D. G. & Connelly, A. Direct estimation of the fiber orientation density function from diffusion-weighted MRI data using spherical deconvolution. NeuroImage, 2004, 23, 1176-1185

* If using 'msmt_csd' algorithm: Jeurissen, B.; Tournier, J.-D.; Dhollander, T.; Connelly, A. & Sijbers, J. Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data. NeuroImage, 2014, 103, 411-426

* If using 'tax' algorithm: Tax, C. M.; Jeurissen, B.; Vos, S. B.; Viergever, M. A. & Leemans, A. Recursive calibration of the fiber response function for spherical deconvolution of diffusion MRI data. NeuroImage, 2014, 86, 67-80

* If using 'tournier' algorithm: Tournier, J.-D.; Calamante, F. & Connelly, A. Determination of the appropriate b value and number of gradient directions for high-angular-resolution diffusion-weighted imaging. NMR Biomedicine, 2013, 26, 1775-1786

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au) and Thijs Dhollander (thijs.dhollander@gmail.com)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

.. _dwi2response_manual:

dwi2response manual
===================

Synopsis
--------

::

    dwi2response manual [ options ] input in_voxels output

-  *input*: The input DWI
-  *in_voxels*: Input voxel selection mask
-  *output*: Output response function text file

Description
-----------

Derive a response function using an input mask image alone (i.e. pre-selected voxels)

Options
-------

Options specific to the 'manual' algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-dirs** Manually provide the fibre direction in each voxel (a tensor fit will be used otherwise)

Options common to all dwi2response algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-shell** The b-value shell(s) to use in response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-lmax** The maximum harmonic degree(s) of response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-mask** Provide an initial mask for response voxel selection

- **-voxels** Output an image showing the final voxel selection(s)

- **-grad** Pass the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Pass the diffusion gradient table in FSL bvecs/bvals format

Standard options
^^^^^^^^^^^^^^^^

- **-continue <TempDir> <LastFile>** Continue the script from a previous execution; must provide the temporary directory path, and the name of the last successfully-generated file

- **-force** Force overwrite of output files if pre-existing

- **-help** Display help information for the script

- **-nocleanup** Do not delete temporary files during script, or temporary directory at script completion

- **-nthreads number** Use this number of threads in MRtrix multi-threaded applications (0 disables multi-threading)

- **-tempdir /path/to/tmp/** Manually specify the path in which to generate the temporary directory

- **-quiet** Suppress all console output during script execution

- **-verbose** Display additional information and progress for every command invoked

- **-debug** Display additional debugging information over and above the verbose output

References
^^^^^^^^^^

* If using 'dhollander' algorithm: Dhollander, T.; Raffelt, D. & Connelly, A. Unsupervised 3-tissue response function estimation from single-shell or multi-shell diffusion MR data without a co-registered T1 image. ISMRM Workshop on Breaking the Barriers of Diffusion MRI, 2016, 5

* If using 'fa' algorithm: Tournier, J.-D.; Calamante, F.; Gadian, D. G. & Connelly, A. Direct estimation of the fiber orientation density function from diffusion-weighted MRI data using spherical deconvolution. NeuroImage, 2004, 23, 1176-1185

* If using 'msmt_csd' algorithm: Jeurissen, B.; Tournier, J.-D.; Dhollander, T.; Connelly, A. & Sijbers, J. Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data. NeuroImage, 2014, 103, 411-426

* If using 'tax' algorithm: Tax, C. M.; Jeurissen, B.; Vos, S. B.; Viergever, M. A. & Leemans, A. Recursive calibration of the fiber response function for spherical deconvolution of diffusion MRI data. NeuroImage, 2014, 86, 67-80

* If using 'tournier' algorithm: Tournier, J.-D.; Calamante, F. & Connelly, A. Determination of the appropriate b value and number of gradient directions for high-angular-resolution diffusion-weighted imaging. NMR Biomedicine, 2013, 26, 1775-1786

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au) and Thijs Dhollander (thijs.dhollander@gmail.com)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

.. _dwi2response_msmt_5tt:

dwi2response msmt_5tt
=====================

Synopsis
--------

::

    dwi2response msmt_5tt [ options ] input in_5tt out_wm out_gm out_csf

-  *input*: The input DWI
-  *in_5tt*: Input co-registered 5TT image
-  *out_wm*: Output WM response text file
-  *out_gm*: Output GM response text file
-  *out_csf*: Output CSF response text file

Description
-----------

Derive MSMT-CSD tissue response functions based on a co-registered five-tissue-type (5TT) image

Options
-------

Options specific to the 'msmt_5tt' algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-dirs** Manually provide the fibre direction in each voxel (a tensor fit will be used otherwise)

- **-fa** Upper fractional anisotropy threshold for GM and CSF voxel selection

- **-pvf** Partial volume fraction threshold for tissue voxel selection

- **-wm_algo algorithm** dwi2response algorithm to use for WM single-fibre voxel selection

Options common to all dwi2response algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-shell** The b-value shell(s) to use in response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-lmax** The maximum harmonic degree(s) of response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-mask** Provide an initial mask for response voxel selection

- **-voxels** Output an image showing the final voxel selection(s)

- **-grad** Pass the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Pass the diffusion gradient table in FSL bvecs/bvals format

Standard options
^^^^^^^^^^^^^^^^

- **-continue <TempDir> <LastFile>** Continue the script from a previous execution; must provide the temporary directory path, and the name of the last successfully-generated file

- **-force** Force overwrite of output files if pre-existing

- **-help** Display help information for the script

- **-nocleanup** Do not delete temporary files during script, or temporary directory at script completion

- **-nthreads number** Use this number of threads in MRtrix multi-threaded applications (0 disables multi-threading)

- **-tempdir /path/to/tmp/** Manually specify the path in which to generate the temporary directory

- **-quiet** Suppress all console output during script execution

- **-verbose** Display additional information and progress for every command invoked

- **-debug** Display additional debugging information over and above the verbose output

References
^^^^^^^^^^

* If using 'dhollander' algorithm: Dhollander, T.; Raffelt, D. & Connelly, A. Unsupervised 3-tissue response function estimation from single-shell or multi-shell diffusion MR data without a co-registered T1 image. ISMRM Workshop on Breaking the Barriers of Diffusion MRI, 2016, 5

* If using 'fa' algorithm: Tournier, J.-D.; Calamante, F.; Gadian, D. G. & Connelly, A. Direct estimation of the fiber orientation density function from diffusion-weighted MRI data using spherical deconvolution. NeuroImage, 2004, 23, 1176-1185

* If using 'msmt_csd' algorithm: Jeurissen, B.; Tournier, J.-D.; Dhollander, T.; Connelly, A. & Sijbers, J. Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data. NeuroImage, 2014, 103, 411-426

* If using 'tax' algorithm: Tax, C. M.; Jeurissen, B.; Vos, S. B.; Viergever, M. A. & Leemans, A. Recursive calibration of the fiber response function for spherical deconvolution of diffusion MRI data. NeuroImage, 2014, 86, 67-80

* If using 'tournier' algorithm: Tournier, J.-D.; Calamante, F. & Connelly, A. Determination of the appropriate b value and number of gradient directions for high-angular-resolution diffusion-weighted imaging. NMR Biomedicine, 2013, 26, 1775-1786

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au) and Thijs Dhollander (thijs.dhollander@gmail.com)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

.. _dwi2response_tax:

dwi2response tax
================

Synopsis
--------

::

    dwi2response tax [ options ] input output

-  *input*: The input DWI
-  *output*: The output response function text file

Description
-----------

Use the Tax et al. (2014) recursive calibration algorithm for single-fibre voxel selection and response function estimation

Options
-------

Options specific to the 'tax' algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-peak_ratio** Second-to-first-peak amplitude ratio threshold

- **-max_iters** Maximum number of iterations

- **-convergence** Percentile change in any RF coefficient required to continue iterating

Options common to all dwi2response algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-shell** The b-value shell(s) to use in response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-lmax** The maximum harmonic degree(s) of response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-mask** Provide an initial mask for response voxel selection

- **-voxels** Output an image showing the final voxel selection(s)

- **-grad** Pass the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Pass the diffusion gradient table in FSL bvecs/bvals format

Standard options
^^^^^^^^^^^^^^^^

- **-continue <TempDir> <LastFile>** Continue the script from a previous execution; must provide the temporary directory path, and the name of the last successfully-generated file

- **-force** Force overwrite of output files if pre-existing

- **-help** Display help information for the script

- **-nocleanup** Do not delete temporary files during script, or temporary directory at script completion

- **-nthreads number** Use this number of threads in MRtrix multi-threaded applications (0 disables multi-threading)

- **-tempdir /path/to/tmp/** Manually specify the path in which to generate the temporary directory

- **-quiet** Suppress all console output during script execution

- **-verbose** Display additional information and progress for every command invoked

- **-debug** Display additional debugging information over and above the verbose output

References
^^^^^^^^^^

* If using 'dhollander' algorithm: Dhollander, T.; Raffelt, D. & Connelly, A. Unsupervised 3-tissue response function estimation from single-shell or multi-shell diffusion MR data without a co-registered T1 image. ISMRM Workshop on Breaking the Barriers of Diffusion MRI, 2016, 5

* If using 'fa' algorithm: Tournier, J.-D.; Calamante, F.; Gadian, D. G. & Connelly, A. Direct estimation of the fiber orientation density function from diffusion-weighted MRI data using spherical deconvolution. NeuroImage, 2004, 23, 1176-1185

* If using 'msmt_csd' algorithm: Jeurissen, B.; Tournier, J.-D.; Dhollander, T.; Connelly, A. & Sijbers, J. Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data. NeuroImage, 2014, 103, 411-426

* If using 'tax' algorithm: Tax, C. M.; Jeurissen, B.; Vos, S. B.; Viergever, M. A. & Leemans, A. Recursive calibration of the fiber response function for spherical deconvolution of diffusion MRI data. NeuroImage, 2014, 86, 67-80

* If using 'tournier' algorithm: Tournier, J.-D.; Calamante, F. & Connelly, A. Determination of the appropriate b value and number of gradient directions for high-angular-resolution diffusion-weighted imaging. NMR Biomedicine, 2013, 26, 1775-1786

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au) and Thijs Dhollander (thijs.dhollander@gmail.com)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

.. _dwi2response_tournier:

dwi2response tournier
=====================

Synopsis
--------

::

    dwi2response tournier [ options ] input output

-  *input*: The input DWI
-  *output*: The output response function text file

Description
-----------

Use the Tournier et al. (2013) iterative algorithm for single-fibre voxel selection and response function estimation

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

- **-shell** The b-value shell(s) to use in response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-lmax** The maximum harmonic degree(s) of response function estimation (single value for single-shell response, comma-separated list for multi-shell response)

- **-mask** Provide an initial mask for response voxel selection

- **-voxels** Output an image showing the final voxel selection(s)

- **-grad** Pass the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Pass the diffusion gradient table in FSL bvecs/bvals format

Standard options
^^^^^^^^^^^^^^^^

- **-continue <TempDir> <LastFile>** Continue the script from a previous execution; must provide the temporary directory path, and the name of the last successfully-generated file

- **-force** Force overwrite of output files if pre-existing

- **-help** Display help information for the script

- **-nocleanup** Do not delete temporary files during script, or temporary directory at script completion

- **-nthreads number** Use this number of threads in MRtrix multi-threaded applications (0 disables multi-threading)

- **-tempdir /path/to/tmp/** Manually specify the path in which to generate the temporary directory

- **-quiet** Suppress all console output during script execution

- **-verbose** Display additional information and progress for every command invoked

- **-debug** Display additional debugging information over and above the verbose output

References
^^^^^^^^^^

* If using 'dhollander' algorithm: Dhollander, T.; Raffelt, D. & Connelly, A. Unsupervised 3-tissue response function estimation from single-shell or multi-shell diffusion MR data without a co-registered T1 image. ISMRM Workshop on Breaking the Barriers of Diffusion MRI, 2016, 5

* If using 'fa' algorithm: Tournier, J.-D.; Calamante, F.; Gadian, D. G. & Connelly, A. Direct estimation of the fiber orientation density function from diffusion-weighted MRI data using spherical deconvolution. NeuroImage, 2004, 23, 1176-1185

* If using 'msmt_csd' algorithm: Jeurissen, B.; Tournier, J.-D.; Dhollander, T.; Connelly, A. & Sijbers, J. Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data. NeuroImage, 2014, 103, 411-426

* If using 'tax' algorithm: Tax, C. M.; Jeurissen, B.; Vos, S. B.; Viergever, M. A. & Leemans, A. Recursive calibration of the fiber response function for spherical deconvolution of diffusion MRI data. NeuroImage, 2014, 86, 67-80

* If using 'tournier' algorithm: Tournier, J.-D.; Calamante, F. & Connelly, A. Determination of the appropriate b value and number of gradient directions for high-angular-resolution diffusion-weighted imaging. NMR Biomedicine, 2013, 26, 1775-1786

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au) and Thijs Dhollander (thijs.dhollander@gmail.com)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

.. _dwi2response:

dwi2response
============

Synopsis
--------

::

    dwi2response [ options ] algorithm ...

-  *algorithm*: Select the algorithm to be used to derive the response function; additional details and options become available once an algorithm is nominated. Options are: dhollander, fa, manual, msmt_5tt, tax, tournier

Description
-----------

Estimate response function(s) for spherical deconvolution

Options
-------

Standard options
^^^^^^^^^^^^^^^^

- **-continue <TempDir> <LastFile>** Continue the script from a previous execution; must provide the temporary directory path, and the name of the last successfully-generated file

- **-force** Force overwrite of output files if pre-existing

- **-help** Display help information for the script

- **-nocleanup** Do not delete temporary files during script, or temporary directory at script completion

- **-nthreads number** Use this number of threads in MRtrix multi-threaded applications (0 disables multi-threading)

- **-tempdir /path/to/tmp/** Manually specify the path in which to generate the temporary directory

- **-quiet** Suppress all console output during script execution

- **-verbose** Display additional information and progress for every command invoked

- **-debug** Display additional debugging information over and above the verbose output

References
^^^^^^^^^^

* If using 'dhollander' algorithm: Dhollander, T.; Raffelt, D. & Connelly, A. Unsupervised 3-tissue response function estimation from single-shell or multi-shell diffusion MR data without a co-registered T1 image. ISMRM Workshop on Breaking the Barriers of Diffusion MRI, 2016, 5

* If using 'fa' algorithm: Tournier, J.-D.; Calamante, F.; Gadian, D. G. & Connelly, A. Direct estimation of the fiber orientation density function from diffusion-weighted MRI data using spherical deconvolution. NeuroImage, 2004, 23, 1176-1185

* If using 'msmt_csd' algorithm: Jeurissen, B.; Tournier, J.-D.; Dhollander, T.; Connelly, A. & Sijbers, J. Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data. NeuroImage, 2014, 103, 411-426

* If using 'tax' algorithm: Tax, C. M.; Jeurissen, B.; Vos, S. B.; Viergever, M. A. & Leemans, A. Recursive calibration of the fiber response function for spherical deconvolution of diffusion MRI data. NeuroImage, 2014, 86, 67-80

* If using 'tournier' algorithm: Tournier, J.-D.; Calamante, F. & Connelly, A. Determination of the appropriate b value and number of gradient directions for high-angular-resolution diffusion-weighted imaging. NMR Biomedicine, 2013, 26, 1775-1786

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au) and Thijs Dhollander (thijs.dhollander@gmail.com)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

