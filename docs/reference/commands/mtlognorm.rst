.. _mtlognorm:

mtlognorm
===================

Synopsis
--------

Multi-tissue informed log-domain intensity normalisation

Usage
--------

::

    mtlognorm [ options ]  input output [ input output ... ]

-  *input output*: list of all input and output tissue compartment files. See example usage in the description. Note that any number of tissues can be normalised

Description
-----------

This command inputs N number of tissue components (e.g. from multi-tissue CSD), and outputs N corrected tissue components. Intensity normalisation is performed by either determining a common global normalisation factor for all tissue types (default) or by normalising each tissue type independently with a single tissue-specific global scale factor.

The -mask option is mandatory, and is optimally provided with a brain mask, such as the one obtained from dwi2mask earlier in the processing pipeline.

Example usage: mtlognorm wm.mif wm_norm.mif gm.mif gm_norm.mif csf.mif csf_norm.mif -mask mask.mif.

Options
-------

-  **-mask image** define the mask to compute the normalisation within. This option is mandatory.

-  **-value number** specify the value to which the summed tissue compartments will be normalised to (Default: sqrt(1/(4*pi)) = 0.282094)

-  **-bias image** output the estimated bias field

-  **-maxiter number** set the number of iterations. Default(15).

-  **-check image** check the final mask used to compute the bias field. This mask excludes outlier regions ignored by the bias field fitting procedure.However, these regions are still corrected for bias fields based on the other image data.

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



**Author:** Thijs Dhollander (thijs.dhollander@gmail.com), Rami Tabbara (rami.tabbara@florey.edu.au) and David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


