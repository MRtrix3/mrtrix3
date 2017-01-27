.. _dwibiascorrect:

dwibiascorrect
==============

Synopsis
--------

::

    dwibiascorrect [ options ] input output

-  *input*: The input image series to be corrected
-  *output*: The output corrected image series

Description
-----------

Perform B1 field inhomogeneity correction for a DWI volume series

Options
-------

Options for the dwibiascorrect script
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-mask** Manually provide a mask image for bias field estimation

- **-bias** Output the estimated bias field

- **-ants** Use ANTS N4 to estimate the inhomogeneity field

- **-fsl** Use FSL FAST to estimate the inhomogeneity field

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

* If using -fast option: Zhang, Y.; Brady, M. & Smith, S. Segmentation of brain MR images through a hidden Markov random field model and the expectation-maximization algorithm. IEEE Transactions on Medical Imaging, 2001, 20, 45-57

* If using -fast option: Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.; Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.; Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.; Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. Advances in functional and structural MR image analysis and implementation as FSL. NeuroImage, 2004, 23, S208-S219

* If using -ants option: Tustison, N.; Avants, B.; Cook, P.; Zheng, Y.; Egan, A.; Yushkevich, P. & Gee, J. N4ITK: Improved N3 Bias Correction. IEEE Transactions on Medical Imaging, 2010, 29, 1310-1320

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.

