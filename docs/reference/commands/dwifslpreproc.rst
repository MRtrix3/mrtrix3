.. _dwifslpreproc:

dwifslpreproc
=============

Synopsis
--------

Perform diffusion image pre-processing using FSL's eddy tool; including inhomogeneity distortion correction using FSL's topup tool if possible

Usage
-----

::

    dwifslpreproc input output [ options ]

-  *input*: The input DWI series to be corrected
-  *output*: The output corrected image series

Description
-----------

This script is intended to provide convenience of use of the FSL software tools topup and eddy for performing DWI pre-processing, by encapsulating some of the surrounding image data and metadata processing steps. It is intended to simply these processing steps for most commonly-used DWI acquisition strategies, whilst also providing support for some more exotic acquisitions. The "example usage" section demonstrates the ways in which the script can be used based on the (compulsory) -rpe_* command-line options.

The "-topup_options" and "-eddy_options" command-line options allow the user to pass desired command-line options directly to the FSL commands topup and eddy. The available options for those commands may vary between versions of FSL; users can interrogate such by querying the help pages of the installed software, and/or the FSL online documentation: (topup) https://fsl.fmrib.ox.ac.uk/fsl/fslwiki/topup/TopupUsersGuide ; (eddy) https://fsl.fmrib.ox.ac.uk/fsl/fslwiki/eddy/UsersGuide

The script will attempt to run the CUDA version of eddy; if this does not succeed for any reason, or is not present on the system, the CPU version will be attempted instead. By default, the CUDA eddy binary found that indicates compilation against the most recent version of CUDA will be attempted; this can be over-ridden by providing a soft-link "eddy_cuda" within your path that links to the binary you wish to be executed.

Note that this script does not perform any explicit registration between images provided to topup via the -se_epi option, and the DWI volumes provided to eddy. In some instances (motion between acquisitions) this can result in erroneous application of the inhomogeneity field during distortion correction. Use of the -align_seepi option is advocated in this scenario, which ensures that the first volume in the series provided to topup is also the first volume in the series provided to eddy, guaranteeing alignment. But a prerequisite for this approach is that the image contrast within the images provided to the -se_epi option must match the b=0 volumes present within the input DWI series: this means equivalent TE, TR and flip angle (note that differences in multi-band factors between two acquisitions may lead to differences in TR).

Example usages
--------------

-   *A basic DWI acquisition, where all image volumes are acquired in a single protocol with fixed phase encoding*::

        $ dwifslpreproc DWI_in.mif DWI_out.mif -rpe_none -pe_dir ap -readout_time 0.55

    Due to use of a single fixed phase encoding, no EPI distortion correction can be applied in this case.

-   *DWIs all acquired with a single fixed phase encoding; but additionally a pair of b=0 images with reversed phase encoding to estimate the inhomogeneity field*::

        $ mrcat b0_ap.mif b0_pa.mif b0_pair.mif -axis 3; dwifslpreproc DWI_in.mif DWI_out.mif -rpe_pair -se_epi b0_pair.mif -pe_dir ap -readout_time 0.72 -align_seepi

    Here the two individual b=0 volumes are concatenated into a single 4D image series, and this is provided to the script via the -se_epi option. Note that with the -rpe_pair option used here, which indicates that the SE-EPI image series contains one or more pairs of b=0 images with reversed phase encoding, the FIRST HALF of the volumes in the SE-EPI series must possess the same phase encoding as the input DWI series, while the second half are assumed to contain the opposite phase encoding direction but identical total readout time. Use of the -align_seepi option is advocated as long as its use is valid (more information in the Description section).

-   *All DWI directions & b-values are acquired twice, with the phase encoding direction of the second acquisition protocol being reversed with respect to the first*::

        $ mrcat DWI_lr.mif DWI_rl.mif DWI_all.mif -axis 3; dwifslpreproc DWI_all.mif DWI_out.mif -rpe_all -pe_dir lr -readout_time 0.66

    Here the two acquisition protocols are concatenated into a single DWI series containing all acquired volumes. The direction indicated via the -pe_dir option should be the direction of phase encoding used in acquisition of the FIRST HALF of volumes in the input DWI series; ie. the first of the two files that was provided to the mrcat command. In this usage scenario, the output DWI series will contain the same number of image volumes as ONE of the acquired DWI series (ie. half of the number in the concatenated series); this is because the script will identify pairs of volumes that possess the same diffusion sensitisation but reversed phase encoding, and perform explicit recombination of those volume pairs in such a way that image contrast in regions of inhomogeneity is determined from the stretched rather than the compressed image.

-   *Any acquisition scheme that does not fall into one of the example usages above*::

        $ mrcat DWI_*.mif DWI_all.mif -axis 3; mrcat b0_*.mif b0_all.mif -axis 3; dwifslpreproc DWI_all.mif DWI_out.mif -rpe_header -se_epi b0_all.mif -align_seepi

    With this usage, the relevant phase encoding information is determined entirely based on the contents of the relevant image headers, and dwifslpreproc prepares all metadata for the executed FSL commands accordingly. This can therefore be used if the particular DWI acquisition strategy used does not correspond to one of the simple examples as described in the prior examples. This usage is predicated on the headers of the input files containing appropriately-named key-value fields such that MRtrix3 tools identify them as such. In some cases, conversion from DICOM using MRtrix3 commands will automatically extract and embed this information; however this is not true for all scanner vendors and/or software versions. In the latter case it may be possible to manually provide these metadata; either using the -json_import command-line option of dwifslpreproc, or the -json_import or one of the -import_pe_* command-line options of MRtrix3's mrconvert command (and saving in .mif format) prior to running dwifslpreproc.

Options
-------

- **-pe_dir PE** Manually specify the phase encoding direction of the input series; can be a signed axis number (e.g. -0, 1, +2), an axis designator (e.g. RL, PA, IS), or NIfTI axis codes (e.g. i-, j, k)

- **-readout_time time** Manually specify the total readout time of the input series (in seconds)

- **-se_epi image** Provide an additional image series consisting of spin-echo EPI images, which is to be used exclusively by topup for estimating the inhomogeneity field (i.e. it will not form part of the output image series)

- **-align_seepi** Achieve alignment between the SE-EPI images used for inhomogeneity field estimation, and the DWIs (more information in Description section)

- **-json_import file** Import image header information from an associated JSON file (may be necessary to determine phase encoding information)

- **-topup_options " TopupOptions"** Manually provide additional command-line options to the topup command (provide a string within quotation marks that contains at least one space, even if only passing a single command-line option to topup)

- **-eddy_options " EddyOptions"** Manually provide additional command-line options to the eddy command (provide a string within quotation marks that contains at least one space, even if only passing a single command-line option to eddy)

- **-eddy_mask image** Provide a processing mask to use for eddy, instead of having dwifslpreproc generate one internally using dwi2mask

- **-eddy_slspec file** Provide a file containing slice groupings for eddy's slice-to-volume registration

- **-eddyqc_text directory** Copy the various text-based statistical outputs generated by eddy, and the output of eddy_qc (if installed), into an output directory

- **-eddyqc_all directory** Copy ALL outputs generated by eddy (including images), and the output of eddy_qc (if installed), into an output directory

Options for specifying the acquisition phase-encoding design; note that one of the -rpe_* options MUST be provided
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-rpe_none** Specify that no reversed phase-encoding image data is being provided; eddy will perform eddy current and motion correction only

- **-rpe_pair** Specify that a set of images (typically b=0 volumes) will be provided for use in inhomogeneity field estimation only (using the -se_epi option)

- **-rpe_all** Specify that ALL DWIs have been acquired with opposing phase-encoding

- **-rpe_header** Specify that the phase-encoding information can be found in the image header(s), and that this is the information that the script should use

Options for importing the diffusion gradient table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-grad** Provide the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Provide the diffusion gradient table in FSL bvecs/bvals format

Options for exporting the diffusion gradient table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-export_grad_mrtrix grad** Export the final gradient table in MRtrix format

- **-export_grad_fsl bvecs bvals** Export the final gradient table in FSL bvecs/bvals format

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify the path in which to generate the scratch directory.

- **-continue <ScratchDir> <LastFile>** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

Standard options
^^^^^^^^^^^^^^^^

- **-info** display information messages.

- **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

- **-debug** display debugging messages.

- **-force** force overwrite of output files.

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

- **-config key value**  *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

- **-help** display this information page and exit.

- **-version** display version information and exit.

References
^^^^^^^^^^

* Andersson, J. L. & Sotiropoulos, S. N. An integrated approach to correction for off-resonance effects and subject movement in diffusion MR imaging. NeuroImage, 2015, 125, 1063-1078

* Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.; Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.; Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.; Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. Advances in functional and structural MR image analysis and implementation as FSL. NeuroImage, 2004, 23, S208-S219

* If performing recombination of diffusion-weighted volume pairs with opposing phase encoding directions: Skare, S. & Bammer, R. Jacobian weighting of distortion corrected EPI data. Proceedings of the International Society for Magnetic Resonance in Medicine, 2010, 5063

* If performing EPI susceptibility distortion correction: Andersson, J. L.; Skare, S. & Ashburner, J. How to correct susceptibility distortions in spin-echo echo-planar images: application to diffusion tensor imaging. NeuroImage, 2003, 20, 870-888

* If including "--repol" in -eddy_options input: Andersson, J. L. R.; Graham, M. S.; Zsoldos, E. & Sotiropoulos, S. N. Incorporating outlier detection and replacement into a non-parametric framework for movement and distortion correction of diffusion MR images. NeuroImage, 2016, 141, 556-572

* If including "--mporder" in -eddy_options input: Andersson, J. L. R.; Graham, M. S.; Drobnjak, I.; Zhang, H.; Filippini, N. & Bastiani, M. Towards a comprehensive framework for movement and distortion correction of diffusion MR images: Within volume movement. NeuroImage, 2017, 152, 450-466

* If using -eddyqc_test or -eddyqc_all option and eddy_quad is installed: Bastiani, M.; Cottaar, M.; Fitzgibbon, S.P.; Suri, S.; Alfaro-Almagro, F.; Sotiropoulos, S.N.; Jbabdi, S.; Andersson, J.L.R. Automated quality control for within and between studies diffusion MRI data using a non-parametric framework for movement and distortion correction. NeuroImage, 2019, 184, 801-812

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2023 the MRtrix3 contributors.

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

