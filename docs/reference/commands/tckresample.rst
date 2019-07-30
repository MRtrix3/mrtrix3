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

Note that if multi-threading is used in this command, the ordering of tracks in the output file is unlikely to match the order of the incoming data. If your application explicitly requires that the order of tracks not change, you should run this command with the option -nthreads 0.

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

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status; alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files (caution: using the same file as input and output might cause unexpected behaviour).

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-config key value**  *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au) and J-Donald Tournier (jdtournier@gmail.com)

**Copyright:** Copyright (c) 2008-2019 the MRtrix3 contributors.

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


