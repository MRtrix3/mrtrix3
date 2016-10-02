.. _mtbin:

mtbin
=====

Synopsis
--------

::

    mtbin [ options ] input_output

-  *input_output*: list of all input and output tissue compartment files. See example usage in the description. Note that any number of tissues can be normalised

Description
-----------

Multi-Tissue Bias field correction and Intensity Normalisation (MTBIN). This script inputs N number of tissue components (from multi-tissue CSD), and outputs N corrected tissue components. Intensity normalisation is performed using a single global scale factor for each tissue type. Example usage: mtbin wm.mif wm_norm.mif gm.mif gm_norm.mif csf.mif csf_norm.mif

Options
-------

Options for the mtbin script
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-mask** define the mask to compute the normalisation within. If not supplied this is estimated automatically

- **-value** specify the value to which the summed tissue compartments will be globally normalised to (Default: sqrt(1/(4*pi) = 0.282)

- **-iter** bias field correction and intensity normalisation is performed iteratively. Specify the number of iterations (Default: 5)

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

--------------



**Author:** David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

