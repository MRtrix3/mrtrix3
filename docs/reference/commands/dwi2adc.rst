.. _dwi2adc:

dwi2adc
===================

Synopsis
--------

Calculate ADC and/or IVIM parameters.

Usage
--------

::

    dwi2adc [ options ]  input output

-  *input*: the input image.
-  *output*: the output image.

Description
-----------

By default, the command will estimate the Apparent Diffusion Coefficient (ADC) using the isotropic mono-exponential model: S(b) = S(0) * exp(-D * b). The output consists of 2 volumes, respectively S(0) and D.

When using the -ivim option, the command will additionally estimate the Intra-Voxel Incoherent Motion (IVIM) parameters f and D', i.e., the perfusion fraction and the pseudo-diffusion coefficient. IVIM assumes a bi-exponential model: S(b) = S(0) * ((1-f) * exp(-D * b) + f * exp(-D' * b)). This command adopts a 2-stage fitting strategy, in which the ADC is first estimated based on the DWI data with b > cutoff, and the other parameters are estimated subsequently. The output consists of 4 volumes, respectively S(0), D, f, and D'.

Note that this command ignores the gradient orientation entirely. This approach is therefore only suited for mean DWI (trace-weighted) images or for DWI data collected with isotropically-distributed gradient directions.

Options
-------

-  **-ivim** also estimate IVIM parameters in 2-stage fit.

-  **-cutoff bval** minimum b-value for ADC estimation in IVIM fit (default = 120 s/mm^2).

DW gradient table import options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad file** Provide the diffusion-weighted gradient scheme used in the acquisition in a text file. This should be supplied as a 4xN text file with each line in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

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

Le Bihan, D.; Breton, E.; Lallemand, D.; Aubin, M.L.; Vignaud, J.; Laval-Jeantet, M. Separation of diffusion and perfusion in intravoxel incoherent motion MR imaging. Radiology, 1988, 168, 497–505.

Jalnefjord, O.; Andersson, M.; Montelius; M.; Starck, G.; Elf, A.; Johanson, V.; Svensson, J.; Ljungberg, M. Comparison of methods for estimation of the intravoxel incoherent motion (IVIM) diffusion coefficient (D) and perfusion fraction (f). Magn Reson Mater Phy, 2018, 31, 715–723.

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** J-Donald Tournier (jdtournier@gmail.com) and Daan Christiaens (daan.christiaens@kuleuven.be)

**Copyright:** Copyright (c) 2008-2024 the MRtrix3 contributors.

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


