.. _dwi2tensor:

dwi2tensor
===================

Synopsis
--------

Diffusion (kurtosis) tensor estimation using iteratively reweighted linear least squares estimator

Usage
--------

::

    dwi2tensor [ options ]  dwi dt

-  *dwi*: the input dwi image.
-  *dt*: the output dt image.

Description
-----------

The tensor coefficients are stored in the output image as follows: volumes 0-5: D11, D22, D33, D12, D13, D23 ; If diffusion kurtosis is estimated using the -dkt option, these are stored as follows: volumes 0-2: W1111, W2222, W3333 ; volumes 3-8: W1112, W1113, W1222, W1333, W2223, W2333 ; volumes 9-11: W1122, W1133, W2233 ; volumes 12-14: W1123, W1223, W1233 ;

Options
-------

-  **-mask image** only perform computation within the specified binary brain mask image.

-  **-b0 image** the output b0 image.

-  **-dkt image** the output dkt image.

-  **-iter integer** number of iterative reweightings (default: 2); set to 0 for ordinary linear least squares.

-  **-predicted_signal image** the predicted dwi image.

DW gradient table import options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad file** Provide the diffusion-weighted gradient scheme used in the acquisition in a text file. This should be supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

-  **-fslgrad bvecs bvals** Provide the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format files. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

-  **-bvalue_scaling mode** specifies whether the b-values should be scaled by the square of the corresponding DW gradient norm, as often required for multi-shell or DSI DW acquisition schemes. The default action can also be set in the MRtrix config file, under the BValueScaling entry. Valid choices are yes/no, true/false, 0/1 (default: true).

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

Veraart, J.; Sijbers, J.; Sunaert, S.; Leemans, A. & Jeurissen, B. Weighted linear least squares estimation of diffusion MRI parameters: strengths, limitations, and pitfalls. NeuroImage, 2013, 81, 335-346

--------------



**Author:** Ben Jeurissen (ben.jeurissen@uantwerpen.be)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


