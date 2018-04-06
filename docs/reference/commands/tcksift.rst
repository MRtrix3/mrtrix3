.. _tcksift:

tcksift
===================

Synopsis
--------

Filter a whole-brain fibre-tracking data set such that the streamline densities match the FOD lobe integrals

Usage
--------

::

    tcksift [ options ]  in_tracks in_fod out_tracks

-  *in_tracks*: the input track file
-  *in_fod*: input image containing the spherical harmonics of the fibre orientation distributions
-  *out_tracks*: the output filtered tracks file

Options
-------

-  **-nofilter** do NOT perform track filtering - just construct the model in order to provide output debugging images

-  **-output_at_counts counts** output filtered track files (and optionally debugging images if -output_debug is specified) at specific numbers of remaining streamlines; provide as comma-separated list of integers

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

-  **-out_selection path** output a text file containing the binary selection of streamlines

Options to control when SIFT terminates filtering
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-term_number value** number of streamlines - continue filtering until this number of streamlines remain

-  **-term_ratio value** termination ratio - defined as the ratio between reduction in cost function, and reduction in density of streamlines.Smaller values result in more streamlines being filtered out.

-  **-term_mu value** terminate filtering once the SIFT proportionality coefficient reaches a given value

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. SIFT: Spherical-deconvolution informed filtering of tractograms. NeuroImage, 2013, 67, 298-312

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2018 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/

MRtrix3 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/


