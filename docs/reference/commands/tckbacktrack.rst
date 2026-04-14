.. _tckbacktrack:

tckbacktrack
===================

Synopsis
--------

Perform streamline truncation based on a 5tt segmentation and re-track streamline terminations

Usage
--------

::

    tckbacktrack [ options ]  tracks_in fod_in 5tt_in tracks_out

-  *tracks_in*: the input file containing the tracks.
-  *fod_in*: the input file containing the FOD image.
-  *5tt_in*: the input file containing the 5tt segmentation.
-  *tracks_out*: the output file containing the tracks generated.

Description
-----------

This command refines an existing whole-brain tractogram by identifying and correcting biologically implausible streamline terminations according to the provided FOD and 5TT images.

The process for each streamline is as follows:

1. Streamline tissue profiling: a 5TT segmentation is used to determine the underlying dominant tissue type for each vertex of the streamline

2. Truncation: identification of a suitable point along the streamline where the terminal segment can be truncated due to conflict between streamline trajectory and 5TT

3. If a suitable truncation point could be identified, that is used as a seed to re-initiate fibre tracking using ACT

Options
-------

Back-tracking/Re-tracking parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-retrack_length_limit value** set the maximum allowed length for re-tracking, expressed as a fraction of the original streamline length (default: 0.25)

-  **-search_length value** set the length of the terminal region to examine for truncation (in mm) (default: 20 mm)

-  **-min_wm_length value** set the minimum length of continuous white matter required to consider a streamline for back-tracking (default: 5 mm)

-  **-min_sgm_length value** set the minimum length of continuous sub-cortical grey matter required to consider a termination valid within that tissue (default: 2 mm)

-  **-truncation_margin value** set the distance to truncate back into the white matter from the identified tissue boundary (default: 2 mm)

Streamlines tractography options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-step size** set the step size of the algorithm in mm (defaults: for first-order algorithms, 0.1 x voxelsize; if using RK4, 0.25 x voxelsize; for iFOD2: 0.5 x voxelsize).

-  **-angle theta** set the maximum angle in degrees between successive steps (defaults: 60 for deterministic algorithms; 15 for iFOD1 / nulldist1; 45 for iFOD2 / nulldist2)

-  **-cutoff value** set the FOD amplitude / fixel size / tensor FA cutoff for terminating tracks (defaults: 0.1 for FOD-based algorithms; 0.1 for fixel-based algorithms; 0.1 for tensor-based algorithms; threshold multiplied by 0.5 when using ACT).

-  **-trials number** set the maximum number of sampling trials at each point (only used for iFOD1 / iFOD2) (default: 1000).

-  **-noprecomputed** do NOT pre-compute legendre polynomial values. Warning: this will slow down the algorithm by a factor of approximately 4.

-  **-rk4** use 4th-order Runge-Kutta integration (slower, but eliminates curvature overshoot in 1st-order deterministic methods)

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

Zanoni, S.; Lv, J.; Smith, R. E. & Calamante, F. Streamline-Based Analysis: A novel framework for tractogram-driven streamline-wise statistical analysis. Proceedings of the International Society for Magnetic Resonance in Medicine, 2025, 4781

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Simone Zanoni (simone.zanoni@sydney.edu.au)

**Copyright:** Copyright (c) 2008-2026 the MRtrix3 contributors.

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


