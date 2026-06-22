.. _tckresample:

tckresample
===================

Synopsis
--------

Resample each streamline in a track file to a new set of vertices

Usage
--------

::

    tckresample [ options ]  in_tracks out_tracks

-  *in_tracks*: the input track file
-  *out_tracks*: the output resampled tracks

Description
-----------

It is necessary to specify precisely ONE of the command-line options for controlling how this resampling takes place; this may be either increasing or decreasing the number of samples along each streamline, or may involve changing the positions of the samples according to some specified trajectory.

Note that because the length of a streamline is calculated based on the sums of distances between adjacent vertices, resampling a streamline to a new set of vertices will typically change the quantified length of that streamline; the magnitude of the difference will typically depend on the discrepancy in the number of vertices, with less vertices leading to a shorter length (due to taking chordal lengths of curved trajectories).

Per-vertex (data-per-vertex) sidecar data, supplied as a track scalar file (.tsf) via the -tsf_in option, are updated to correspond to the output vertices. For the vertex-subset-preserving modes (-downsample, -endpoints), each scalar is sub-sampled to the retained vertices. For the interpolating modes (-upsample, -step_size, -num_points, -line, -arc), which invent new vertex positions, the per-vertex data cannot meaningfully be carried and are dropped (with a warning); in that circumstance the -tsf_out option has no effect. Per-streamline weights (-tck_weights_in/out) pass through unchanged in every mode.

Options
-------

Streamline resampling options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-upsample ratio** increase the density of points along the length of each streamline by some factor (may improve mapping streamlines to ROIs, and/or visualisation)

-  **-downsample ratio** increase the density of points along the length of each streamline by some factor (decreases required storage space)

-  **-step_size value** re-sample the streamlines to a desired step size (in mm)

-  **-num_points count** re-sample each streamline to a fixed number of points

-  **-endpoints** only output the two endpoints of each streamline

-  **-line num start end** resample tracks at 'num' equidistant locations along a line between 'start' and 'end' (specified as comma-separated 3-vectors in scanner coordinates)

-  **-arc num start mid end** resample tracks at 'num' equidistant locations along a circular arc specified by points 'start', 'mid' and 'end' (specified as comma-separated 3-vectors in scanner coordinates)

Options for handling per-vertex (data-per-vertex) sidecar data
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-tsf_in path** an input track scalar file (.tsf), one scalar per vertex of the input tractogram, to be resampled to correspond to the output vertices (only the vertex-subset modes -downsample / -endpoints preserve it; interpolating modes drop it with a warning)

-  **-tsf_out path** the output track scalar file (.tsf) corresponding to -tsf_in (ignored for the interpolating modes, which drop per-vertex data)

Options for handling per-streamline (data-per-streamline) weights
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-tck_weights_in spec** specify the streamline weights: either a standalone scalar file, or "[<tractogram>]::<field>" naming a per-streamline field of the input tractogram (an empty <tractogram>, i.e. "::<field>", refers to the command's own input tractogram, which is the only way to name a field of a piped input)

-  **-tck_weights_out spec** specify where to write the output streamline weights: either a standalone scalar file, or "[<tractogram>]::<field>" naming a per-streamline field of the output tractogram (an empty <tractogram>, i.e. "::<field>", refers to the command's own output tractogram, which is the only way to name a field of a piped output)

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status; alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages & debug input data.

-  **-force** force overwrite of output files (caution: using the same file as input and output might cause unexpected behaviour).

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-config key value** *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au) and J-Donald Tournier (jdtournier@gmail.com)

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


