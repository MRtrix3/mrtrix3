.. _tensor2metric:

tensor2metric
===========

Synopsis
--------

::

    tensor2metric [ options ]  tensor

-  *tensor*: the input tensor image.

Description
-----------

Generate maps of tensor-derived parameters.

Options
-------

-  **-adc image** compute the mean apparent diffusion coefficient (ADC) of the diffusion tensor. (sometimes also referred to as the mean diffusivity (MD))

-  **-fa image** compute the fractional anisotropy (FA) of the diffusion tensor.

-  **-ad image** compute the axial diffusivity (AD) of the diffusion tensor. (equivalent to the principal eigenvalue)

-  **-rd image** compute the radial diffusivity (RD) of the diffusion tensor. (equivalent to the mean of the two non-principal eigenvalues)

-  **-cl image** compute the linearity metric of the diffusion tensor. (one of the three Westin shape metrics)

-  **-cp image** compute the planarity metric of the diffusion tensor. (one of the three Westin shape metrics)

-  **-cs image** compute the sphericity metric of the diffusion tensor. (one of the three Westin shape metrics)

-  **-value image** compute the selected eigenvalue(s) of the diffusion tensor.

-  **-vector image** compute the selected eigenvector(s) of the diffusion tensor.

-  **-num sequence** specify the desired eigenvalue/eigenvector(s). Note that several eigenvalues can be specified as a number sequence. For example, '1,3' specifies the principal (1) and minor (3) eigenvalues/eigenvectors (default = 1).

-  **-modulate choice** specify how to modulate the magnitude of the eigenvectors. Valid choices are: none, FA, eigval (default = FA).

-  **-mask image** only perform computation within the specified binary brain mask image.

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

References
^^^^^^^^^^

Basser, P. J.; Mattiello, J. & Lebihan, D. MR diffusion tensor spectroscopy and imaging. Biophysical Journal, 1994, 66, 259-267

Westin, C. F.; Peled, S.; Gudbjartsson, H.; Kikinis, R. & Jolesz, F. A. Geometrical diffusion measures for MRI from tensor basis analysis. Proc Intl Soc Mag Reson Med, 1997, 5, 1742

--------------



**Author:** Thijs Dhollander (thijs.dhollander@gmail.com) & Ben Jeurissen (ben.jeurissen@uantwerpen.be) & J-Donald Tournier (jdtournier@gmail.com)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

