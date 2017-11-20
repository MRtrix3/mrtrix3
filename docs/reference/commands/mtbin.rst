.. _mtbin:

mtbin
===================

Synopsis
--------

Multi-Tissue Bias field correction and Intensity Normalisation (WARNING: deprecated).

Usage
--------

::

    mtbin [ options ]  input output [ input output ... ]

-  *input output*: list of all input and output tissue compartment files. See example usage in the description. Note that any number of tissues can be normalised

Description
-----------

WARNING: this command is deprecated and may produce highly inappropriate results in several cases. Not recommended and at your own discretion. Please use the new mtnormalise command instead for reliable results.

Options
-------

-  **-mask image** define the mask to compute the normalisation within. This option is mandatory.

-  **-value number** specify the value to which the summed tissue compartments will be normalised to (Default: sqrt(1/(4*pi)) = 0.282094)

-  **-bias image** output the estimated bias field

-  **-independent** intensity normalise each tissue type independently

-  **-maxiter number** set the maximum number of iterations. Default(100). It will stop before the max iterations if convergence is detected

-  **-check image** check the final mask used to compute the bias field. This mask excludes outlier regions ignored by the bias field fitting procedure. However, these regions are still corrected for bias fields based on the other image data.

-  **-override** consciously use this deprecated command. Not recommended and at your own discretion.

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

-  **-failonwarn** terminate program if a warning is produced

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------



**Author:** David Raffelt (david.raffelt@florey.edu.au), Rami Tabbara (rami.tabbara@florey.edu.au) and Thijs Dhollander (thijs.dhollander@gmail.com)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


