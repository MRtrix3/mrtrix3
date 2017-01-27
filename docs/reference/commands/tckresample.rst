.. _tckresample:

tckresample
===========

Synopsis
--------

::

    tckresample [ options ]  in_tracks out_tracks

-  *in_tracks*: the input track file
-  *out_tracks*: the output resampled tracks

Description
-----------

Resample each streamline to a new set of vertices. 

This may be either increasing or decreasing the number of samples along each streamline, or changing the positions of the samples according to some specified trajectory.

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

-  **-quiet** do not display information messages or progress status.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

-  **-failonwarn** terminate program if a warning is produced

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au) and J-Donald Tournier (jdtournier@gmail.com)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.

