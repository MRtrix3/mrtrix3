MRtrix 0.2 equivalent commands
==============================

For those users moving to *MRtrix3* from the old MRtrix 0.2.x software, this list provides the equivalent command names for the functionalities that they are accustomed to from the older version of the software. The new command naming scheme was carefully designed, so we hope people agree that it makes sense, and allows users to easily find the command functionalities available that are relevant for the data they are processing.

Further information can be found on these commands either through the `documentation <commands_list>`__, or by typing the binary name at the command-line with no arguments to access the help file for that command.

============            =======                      ========
MRtrix 0.2.x            MRtrix3                      Comments
-------------------------------------------------------------
``average``             `mrmath`_                   Use ``mean`` statistic as second argument, and ``-axis`` option
``cat_tracks``          `tckedit`_                  Simply provide multiple input track files to the command
``cleanup_ANTS_warp``   *no equivalent*
``csdeconv``            `dwi2fod`_
``dicom_filename``      *no equivalent*             See ``dcminfo``
``dir2amp``             `peaks2amp`_
``disp_profile``        `shview`_
``dwi2SH``              `amp2sh`_
``dwi2tensor``          `dwi2tensor`_
``erode``               `maskfilter`_               Specify ``erode`` or ``dilate`` algorithm as second argument
``estimate_response``   `dwi2response`_             The ``manual`` algorithm in `dwi2response`_ provides a similar functionality to the ``estimate_response`` command in MRtrix 0.2. The `dwi2response`_ script also has a number of algorithms for automatically selecting single-fibre voxels in order to estimate the response function; see the relevant help page
``filter_tracks``       `tckedit`_
``find_SH_peaks``       `sh2peaks`_
``gen_ROI``             *no equivalent* 
``gen_WM_mask``         *no equivalent*             Use of this command was discouraged and so it has been discontinued
``gen_unit_warp``       `warpinit`_
``gendir``              `dirgen`_                   The electrostatic repulsion algorithm now only uses an exponent of 2 by default, rather than increasing in power over iterations; this was found to give poorer minimum-angle values, but superior conditioning of direction schemes
``import_tracks``       `tckconvert`_
``median3D``            `mrfilter`_                 Specify ``median`` algorithm as second argument
``mrabs``               `mrcalc`_                   Use ``-abs`` operator
``mradd``               `mrcalc`_ *or* `mrmath`_    E.g. ``mrcalc A.mif B.mif -add out.mif`` or ``mrmath A.mif B.mif sum out.mif``
``mrcat``               `mrcat`_
``mrconvert``           `mrconvert`_
``mrinfo``              `mrinfo`_
``mrmult``              `mrcalc`_ *or* `mrmath`_    E.g. ``mrcalc A.mif B.mif -mult out.mif`` or ``mrmath A.mif B.mif product out.mif``
``mrstats``             `mrstats`_
``mrtransform``         `mrtransform`_
``mrview``              `mrview`_
``normalise_tracks``    `tcknormalise`_
``read_dicom``          `dcminfo`_
``read_ximg``           *no equivalent*
``resample_tracks``     `tcksample`_
``sample_tracks``       `tcksample`_
``sdeconv``             `dwi2fod`_
``select_tracks``       `tckedit`_
``streamtrack``         `tckgen`_
``tensor2ADC``          `tensor2metric`_            Use ``-adc`` output option
``tensor2FA``           `tensor2metric`_            Use ``-fa`` output option
``tensor2vector``       `tensor2metric`_            Use ``-vector`` output option
``tensor_metric``       `tensor2metric`_
``threshold``           `mrthreshold`_              Note that automatic threshold parameter determination (i.e. if you don't explicitly provide an option to specify how the thresholding should be performed) is done using a different heuristic to that in the MRtrix 0.2 command
``track_info``          `tckinfo`_
``tracks2prob``         `tckmap`_
``tracks2vtk``          `tckconvert`_
``truncate_tracks``     `tckedit`_

