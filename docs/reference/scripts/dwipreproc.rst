.. _dwipreproc:

dwipreproc
==========

Synopsis
--------

::

    dwipreproc [ options ] pe_dir input output

-  *pe_dir*: The phase encode direction; can be a signed axis number (e.g. -0, 1, +2) or a code (e.g. AP, LR, IS)
-  *input*: The input DWI series to be corrected
-  *output*: The output corrected image series

Description
-----------

Perform diffusion image pre-processing using FSL's eddy tool; including inhomogeneity distortion correction using FSL's topup tool if possible

Options
-------

Options for passing reversed phase-encode data; one of these options MUST be provided
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-rpe_none** Specify explicitly that no reversed phase-encoding image data is provided; eddy will perform eddy current and motion correction only

- **-rpe_pair forward reverse** Provide a pair of images to use for inhomogeneity field estimation; note that the FIRST of these two images must have the same phase-encode direction as the input DWIs

- **-rpe_all input_revpe** Provide a second DWI series identical to the input series, that has the opposite phase encoding; these will be combined in the output image

Options for the dwipreproc script
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-cuda** Use the CUDA version of eddy

- **-grad** Provide a gradient table in MRtrix format

- **-fslgrad bvecs bvals** Provide a gradient table in FSL bvecs/bvals format

- **-export_grad_mrtrix grad** Export the final gradient table in MRtrix format

- **-export_grad_fsl bvecs bvals** Export the final gradient table in FSL bvecs/bvals format

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

* Andersson, J. L. & Sotiropoulos, S. N. An integrated approach to correction for off-resonance effects and subject movement in diffusion MR imaging. NeuroImage, 2015, 125, 1063-1078

* Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.; Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.; Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.; Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. Advances in functional and structural MR image analysis and implementation as FSL. NeuroImage, 2004, 23, S208-S219

* If using -rpe_all option: Skare, S. & Bammer, R. Jacobian weighting of distortion corrected EPI data. Proceedings of the International Society for Magnetic Resonance in Medicine, 2010, 5063

* If using -rpe_pair or -rpe_all options: Andersson, J. L.; Skare, S. & Ashburner, J. How to correct susceptibility distortions in spin-echo echo-planar images: application to diffusion tensor imaging. NeuroImage, 2003, 20, 870-888

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public 
License, v. 2.0. If a copy of the MPL was not distributed with this 
file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

