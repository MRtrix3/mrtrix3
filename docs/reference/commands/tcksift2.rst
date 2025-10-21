.. _tcksift2:

tcksift2
===================

Synopsis
--------

Optimise per-streamline cross-section multipliers to match a whole-brain tractogram to fixel-wise fibre densities

Usage
--------

::

    tcksift2 [ options ]  in_tracks in_fod out_weights

-  *in_tracks*: the input track file
-  *in_fod*: input image containing the spherical harmonics of the fibre orientation distributions
-  *out_weights*: output text file containing the weighting factor for each streamline

Description
-----------

Interpretation of not just the relative magnitudes of the output weights of different streamlines, but their ABSOLUTE magnitude, depends on the presence or absence of any modulations applied to those values; by the tcksift2 command itself, and/or other experimental factors applied, whether implicit or explicit. This has been termed "inter-subject connection density normalisation". Within the scope of the tcksift2 command, some control of this normalisation is available by specifying the units of those output weights. The options available for these units, and their corresponding interpretations, are described in further detail in the following paragraphs.

- "NOS" (Number Of Streamlines) / "none": No explicit scaling of the output streamline weights is performed. A key component of the SIFT model as originally devised was to scale the contributions of all streamlines by proportionality coefficient mu, to facilitate direct comparison of tractogram and fixel-wise fibre densities. This is therefore the "native" form in which these streamline weights are computed. In the contex of output of the SIFT2 method, this makes the per-streamline weights approximately centred around unity, such that the overall magnitude of inter-areal connection weights will be comparable to that of the number-of-streamlines metric. This was the behaviour of the tcksift2 command prior to software version 3.1.0.

- "AFD/mm" / "AFD.mm-1", "AFD.mm^-1": The streamline weights in their native representation are multiplied by SIFT model proportionality coefficient mu as they are exported to file. These values encode the AFD per millimetre of length that is contributed to the model by that streamline. Only under specific circumstances does utilising these units permit direct comparison of Fibre Bundle Capacity (FBC) between reconstructions: a) Use of common response function(s); b) Having used some mechanism for global intensity normalisation (as required for any analysis of AFD); c) All DWI data have the same spatial resolution.

- "mm2" / "mm^2": The streamline weights in their native representation are multiplied both by SIFT model proportionality coefficient mu and by the voxel volume in mm^3 as they are exported to file. These units interpret the fixel-wise AFD values as volume fractions (despite the fact that these values do not have an upper bound of 1.0), such that the streamline weights may be interpreted as a physical fibre cross-sectional area in units of mm^2; each streamline therefore contributes some fibre volume per unit length. Only under specific circumstances does utilising these units permit direct comparison of Fibre Bundle Capacity (FBC) between reconstructions: a) Use of common response function(s); b) Having used some mechanism for global intensity normalisation (as required for any analysis of AFD). Unlike the AFD/mm units however, streamline weights exported in these units are invariant to the resolution of the FOD voxel grid used in the SIFT2 optimisation.

Options
-------

-  **-units choice** specify the physical units for the output streamline weights (see Description)

Options for setting the processing mask for the SIFT fixel-streamlines comparison model
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-proc_mask image** provide an image containing the processing mask weights for the model; image spatial dimensions must match the fixel image

-  **-act image** use an ACT five-tissue-type segmented anatomical image to derive the processing mask

Options affecting the SIFT model
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-fd_scale_gm** provide this option (in conjunction with -act) to heuristically downsize the fibre density estimates based on the presence of GM in the voxel. This can assist in reducing tissue interface effects when using a single-tissue deconvolution algorithm

-  **-no_dilate_lut** do NOT dilate FOD lobe lookup tables; only map streamlines to FOD lobes if the precise tangent lies within the angular spread of that lobe

-  **-make_null_lobes** add an additional FOD lobe to each voxel, with zero integral, that covers all directions with zero / negative FOD amplitudes

-  **-remove_untracked** remove FOD lobes that do not have any streamline density attributed to them; this improves filtering slightly, at the expense of longer computation time (and you can no longer trivially do quantitative comparisons between reconstructions if this is enabled)

-  **-fd_thresh value** fibre density threshold; exclude an FOD lobe from filtering processing if its integral is less than this amount (streamlines will still be mapped to it, but it will not contribute to the cost function or the filtering)

Options to make SIFT provide additional output files
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-csv file** output statistics of execution per iteration to a .csv file

-  **-out_mu file** output the final value of SIFT proportionality coefficient mu to a text file

-  **-output_debug dirpath** write to a directory various output images for assessing & debugging performance etc.

-  **-out_coeffs path** output text file containing the weighting coefficient for each streamline

Regularisation options for SIFT2
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-reg_tikhonov value** provide coefficient for regularising streamline weighting coefficients (Tikhonov regularisation) (default: 0)

-  **-reg_tv value** provide coefficient for regularising variance of streamline weighting coefficient to fixels along its length (Total Variation regularisation) (default: 0.1)

Options for controlling the SIFT2 optimisation algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-min_td_frac fraction** minimum fraction of the FOD integral reconstructed by streamlines; if the reconstructed streamline density is below this fraction, the fixel is excluded from optimisation (default: 0.1)

-  **-min_iters count** minimum number of iterations to run before testing for convergence; this can prevent premature termination at early iterations if the cost function increases slightly (default: 10)

-  **-max_iters count** maximum number of iterations to run before terminating program (default: 1000)

-  **-min_factor factor** minimum weighting factor for an individual streamline; if the factor falls below this number, the streamline will be rejected entirely (factor set to zero) (default: 0)

-  **-min_coeff coeff** minimum weighting coefficient for an individual streamline; similar to the '-min_factor' option, but using the exponential coefficient basis of the SIFT2 model; these parameters are related as: factor = e^(coeff). Note that the -min_factor and -min_coeff options are mutually exclusive; you can only provide one. (default: -inf)

-  **-max_factor factor** maximum weighting factor that can be assigned to any one streamline (default: inf)

-  **-max_coeff coeff** maximum weighting coefficient for an individual streamline; similar to the '-max_factor' option, but using the exponential coefficient basis of the SIFT2 model; these parameters are related as: factor = e^(coeff). Note that the -max_factor and -max_coeff options are mutually exclusive; you can only provide one. (default: inf)

-  **-max_coeff_step step** maximum change to a streamline's weighting coefficient in a single iteration (default: 1)

-  **-min_cf_decrease frac** minimum decrease in the cost function (as a fraction of the initial value) that must occur each iteration for the algorithm to continue (default: 2.5e-05)

-  **-linear** perform a linear estimation of streamline weights, rather than the standard non-linear optimisation (typically does not provide as accurate a model fit; but only requires a single pass)

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

Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. SIFT2: Enabling dense quantitative assessment of brain white matter connectivity using streamlines tractography. NeuroImage, 2015, 119, 338-351

Smith, RE; Raffelt, D; Tournier, J-D; Connelly, A. Quantitative Streamlines Tractography: Methods and Inter-Subject Normalisation. OHBM Aperture, doi: 10.52294/ApertureNeuro.2022.2.NEOD9565.

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2025 the MRtrix3 contributors.

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


