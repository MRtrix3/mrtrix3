.. _dwipreproc:

dwipreproc
==========

Synopsis
--------

Perform diffusion image pre-processing using FSL's eddy tool; including inhomogeneity distortion correction using FSL's topup tool if possible

Usage
--------

::

    dwipreproc input output [ options ]

-  *input*: The input DWI series to be corrected
-  *output*: The output corrected image series

Description
-----------

Note that this script does not perform any explicit registration between images provided to topup via the -se_epi option, and the DWI volumes provided to eddy. In some instances (motion between acquisitions) this can result in erroneous application of the inhomogeneity field during distortion correction. If this could potentially be a problem for your data, a possible solution is to insert the first b=0 DWI volume to be the first volume of the image file provided via the -se_epi option. This will hopefully be addressed within the script itself in a future update.

Options
-------

Options for specifying the acquisition phase-encoding design; note that one of the -rpe_* options MUST be provided
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-rpe_none** Specify that no reversed phase-encoding image data is being provided; eddy will perform eddy current and motion correction only

- **-rpe_pair** Specify that a set of images (typically b=0 volumes) will be provided for use in inhomogeneity field estimation only (using the -se_epi option). It is assumed that the FIRST volume(s) of this image has the SAME phase-encoding direction as the input DWIs, and the LAST volume(s) has precisely the OPPOSITE phase encoding

- **-rpe_all** Specify that ALL DWIs have been acquired with opposing phase-encoding; this information will be used to perform a recombination of image volumes (each pair of volumes with the same b-vector but different phase encoding directions will be combined together into a single volume). It is assumed that the SECOND HALF of the volumes in the input DWIs have corresponding diffusion sensitisation directions to the FIRST HALF, but were acquired using precisely the opposite phase-encoding direction

- **-rpe_header** Specify that the phase-encoding information can be found in the image header(s), and that this is the information that the script should use

Other options for the dwipreproc script
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-pe_dir PE** Manually specify the phase encoding direction of the input series; can be a signed axis number (e.g. -0, 1, +2), an axis designator (e.g. RL, PA, IS), or NIfTI axis codes (e.g. i-, j, k)

- **-readout_time time** Manually specify the total readout time of the input series (in seconds)

- **-se_epi file** Provide an additional image series consisting of spin-echo EPI images, which is to be used exclusively by topup for estimating the inhomogeneity field (i.e. it will not form part of the output image series)

- **-json_import JSON_file** Import image header information from an associated JSON file (may be necessary to determine phase encoding information)

- **-eddy_options Options** Manually provide additional command-line options to the eddy command

- **-cuda** Use the CUDA version of eddy (if available)

Options for importing the diffusion gradient table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-grad** Provide a gradient table in MRtrix format

- **-fslgrad bvecs bvals** Provide a gradient table in FSL bvecs/bvals format

Options for exporting the diffusion gradient table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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

- **-info** Display additional information and progress for every command invoked

- **-debug** Display additional debugging information over and above the output of -info

References
^^^^^^^^^^

* Andersson, J. L. & Sotiropoulos, S. N. An integrated approach to correction for off-resonance effects and subject movement in diffusion MR imaging. NeuroImage, 2015, 125, 1063-1078

* Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.; Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.; Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.; Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. Advances in functional and structural MR image analysis and implementation as FSL. NeuroImage, 2004, 23, S208-S219

* If performing recombination of diffusion-weighted volume pairs with opposing phase encoding directions: Skare, S. & Bammer, R. Jacobian weighting of distortion corrected EPI data. Proceedings of the International Society for Magnetic Resonance in Medicine, 2010, 5063

* If performing EPI susceptibility distortion correction: Andersson, J. L.; Skare, S. & Ashburner, J. How to correct susceptibility distortions in spin-echo echo-planar images: application to diffusion tensor imaging. NeuroImage, 2003, 20, 870-888

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

