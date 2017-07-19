.. _tcksift2:

tcksift2
===================

Synopsis
--------

Successor to the SIFT method; instead of removing streamlines, use an EM framework to find an appropriate cross-section multiplier for each streamline

Usage
--------

::

    tcksift2 [ options ]  in_tracks in_fod out_weights

-  *in_tracks*: the input track file
-  *in_fod*: input image containing the spherical harmonics of the fibre orientation distributions
-  *out_weights*: output text file containing the weighting factor for each streamline

Options
-------

Options for setting the processing mask for the SIFT fixel-streamlines comparison model
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-proc_mask image** provide an image containing the processing mask weights for the model; image spatial dimensions must match the fixel image

-  **-act image** use an ACT five-tissue-type segmented anatomical image to derive the processing mask

Options affecting the SIFT model
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-fd_scale_gm** provide this option (in conjunction with -act) to heuristically downsize the fibre density estimates based on the presence of GM in the voxel. This can assist in reducing tissue interface effects when using a single-tissue deconvolution algorithm

-  **-no_dilate_lut** do NOT dilate FOD lobe lookup tables; only map streamlines to FOD lobes if the precise tangent lies within the angular spread of that lobe

-  **-make_null_lobes** add an additional FOD lobe to each voxel, with zero integral, that covers all directions with zero / negative FOD amplitudes

-  **-remove_untracked** remove FOD lobes that do not have any streamline density attributed to them; this improves filtering slightly, at the expense of longer computation time (and you can no longer do quantitative comparisons between reconstructions if this is enabled)

-  **-fd_thresh value** fibre density threshold; exclude an FOD lobe from filtering processing if its integral is less than this amount (streamlines will still be mapped to it, but it will not contribute to the cost function or the filtering)

Options to make SIFT provide additional output files
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-csv file** output statistics of execution per iteration to a .csv file

-  **-out_mu file** output the final value of SIFT proportionality coefficient mu to a text file

-  **-output_debug** provide various output images for assessing & debugging performace etc.

-  **-out_coeffs path** output text file containing the weighting coefficient for each streamline

Regularisation options for SIFT2
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-reg_tikhonov value** provide coefficient for regularising streamline weighting coefficients (Tikhonov regularisation) (default: 0)

-  **-reg_tv value** provide coefficient for regularising variance of streamline weighting coefficient to fixels along its length (Total Variation regularisation) (default: 0.1)

Options for controlling the SIFT2 optimisation algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-min_td_frac fraction** minimum fraction of the FOD integral reconstructed by streamlines; if the reconstructed streamline density is below this fraction, the fixel is excluded from optimisation (default: 0.1)

-  **-min_iters count** minimum number of iterations to run before testing for convergence; this can prevent premature termination at early iterations if the cost function increases slightly (default: 10)

-  **-max_iters count** maximum number of iterations to run before terminating program

-  **-min_factor factor** minimum weighting factor for an individual streamline; if the factor falls below this number the streamline will be rejected entirely (factor set to zero) (default: 0)

-  **-min_coeff coeff** minimum weighting coefficient for an individual streamline; similar to the '-min_factor' option, but using the exponential coefficient basis of the SIFT2 model; these parameters are related as: factor = e^(coeff). Note that the -min_factor and -min_coeff options are mutually exclusive - you can only provide one. (default: -inf)

-  **-max_factor factor** maximum weighting factor that can be assigned to any one streamline (default: inf)

-  **-max_coeff coeff** maximum weighting coefficient for an individual streamline; similar to the '-max_factor' option, but using the exponential coefficient basis of the SIFT2 model; these parameters are related as: factor = e^(coeff). Note that the -max_factor and -max_coeff options are mutually exclusive - you can only provide one. (default: inf)

-  **-max_coeff_step step** maximum change to a streamline's weighting coefficient in a single iteration (default: 1)

-  **-min_cf_decrease frac** minimum decrease in the cost function (as a fraction of the initial value) that must occur each iteration for the algorithm to continue (default: 2.5e-05)

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

References
^^^^^^^^^^

Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. SIFT2: Enabling dense quantitative assessment of brain white matter connectivity using streamlines tractography. NeuroImage, 2015, 119, 338-351

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


