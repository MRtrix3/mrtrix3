.. _mtbin:

mtbin
===========

Synopsis
--------

::

    mtbin [ options ]  input output [ input output ... ]

-  *input output*: list of all input and output tissue compartment files. See example usage in the description. Note that any number of tissues can be normalised

Description
-----------

Multi-Tissue Bias field correction and Intensity Normalisation (MTBIN). This script inputs N number of tissue components (e.g. from multi-tissue CSD), and outputs N corrected tissue components. Intensity normalisation is performed by either normalising each tissue type independently with a single global scale factor per tissue or determining a normalise all tissues with the same scale factor (default). Example usage: mtbin wm.mif wm_norm.mif gm.mif gm_norm.mif csf.mif csf_norm.mif. The estimated multiplicative bias field is guaranteed to have a mean of 1 over all voxels within the mask

Options
-------

-  **-mask image** define the mask to compute the normalisation within. If not supplied this is estimated automatically

-  **-value number** specify the value to which the summed tissue compartments will be to (Default: sqrt(1/(4*pi) = 0.282)

-  **-bias image** output the estimated bias field

-  **-independent** intensity normalise each tissue type independently

-  **-maxiter number** set the maximum number of iterations. Default(100). It will stop before the max iterations if convergence is detected

-  **-check image** check the automatically computed mask

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

--------------



**Author:** David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

