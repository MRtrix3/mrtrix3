.. _dwi2tensor:

dwi2tensor
===================

Synopsis
--------

Diffusion (kurtosis) tensor estimation

Usage
--------

::

    dwi2tensor [ options ]  dwi dt

-  *dwi*: the input dwi image.
-  *dt*: the output dt image.

Description
-----------

By default, the diffusion tensor (and optionally its kurtosis) is fitted to the log-signal in two steps: firstly, using weighted least-squares (WLS) with weights based on the empirical signal intensities; secondly, by further iterated weighted least-squares (IWLS) with weights determined by the signal predictions from the previous iteration (by default, 2 iterations will be performed). This behaviour can be altered in two ways:

* The -ols option will cause the first fitting step to be performed using ordinary least-squares (OLS); that is, all measurements contribute equally to the fit, instead of the default behaviour of weighting based on the empirical signal intensities.

* The -iter option controls the number of iterations of the IWLS prodedure. If this is set to zero, then the output model parameters will be those resulting from the first fitting step only: either WLS by default, or OLS if the -ols option is used in conjunction with -iter 0.

The tensor coefficients are stored in the output image as follows: |br|
volumes 0-5: D11, D22, D33, D12, D13, D23

If diffusion kurtosis is estimated using the -dkt option, these are stored as follows: |br|
volumes 0-2: W1111, W2222, W3333 |br|
volumes 3-8: W1112, W1113, W1222, W1333, W2223, W2333 |br|
volumes 9-11: W1122, W1133, W2233 |br|
volumes 12-14: W1123, W1223, W1233

Options
-------

-  **-ols** perform initial fit using an ordinary least-squares (OLS) fit (see Description).

-  **-mask image** only perform computation within the specified binary brain mask image.

-  **-b0 image** the output b0 image.

-  **-dkt image** the output dkt image.

-  **-iter integer** number of iterative reweightings for IWLS algorithm (default: 2) (see Description).

-  **-predicted_signal image** the predicted dwi image.

DW gradient table import options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad file** Provide the diffusion-weighted gradient scheme used in the acquisition in a text file. This should be supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

-  **-fslgrad bvecs bvals** Provide the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format files. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

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

References based on fitting algorithm used:

* OLS, WLS: |br|
  Basser, P.J.; Mattiello, J.; LeBihan, D. Estimation of the effective self-diffusion tensor from the NMR spin echo. J Magn Reson B., 1994, 103, 247–254.

* IWLS: |br|
  Veraart, J.; Sijbers, J.; Sunaert, S.; Leemans, A. & Jeurissen, B. Weighted linear least squares estimation of diffusion MRI parameters: strengths, limitations, and pitfalls. NeuroImage, 2013, 81, 335-346

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Ben Jeurissen (ben.jeurissen@uantwerpen.be)

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


