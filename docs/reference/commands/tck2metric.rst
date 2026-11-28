.. _tck2metric:

tck2metric
===================

Synopsis
--------

Compute one or more per-streamline metrics for a tractogram

Usage
--------

::

    tck2metric [ options ]  tracks_in

-  *tracks_in*: the input track file

Description
-----------

Each requested metric is derived from a single serialised pass through the input tractogram, so that multiple metrics can be exported simultaneously without re-reading the data.

At least one metric export option must be specified.

Streamline length can be exported per-streamline to a vector file using the -length option, or summarised as a single mean value reported to stdout using the -mean_length option.

Streamline curvature is estimated using a smooth arc-length-based local fit; the -mean_curvature option exports the per-streamline mean curvature to a vector file, whereas the -vertex_curvature option exports the per-vertex curvature to a track scalar (.tsf) file, the latter being written in the same order as the streamlines of the input tractogram.

Options
-------

Options for exporting streamline length
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-length path** export the length of each streamline to a vector file

-  **-mean_length** compute the mean streamline length and report it to stdout

-  **-ignorezero** do not generate a warning if the track file contains streamlines with zero length

Options for exporting streamline curvature
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-mean_curvature path** export the per-streamline mean curvature (1/mm) to a vector file

-  **-vertex_curvature path** export the per-vertex curvature (1/mm) to a track scalar (.tsf) file

-  **-tck_weights_in path** specify a text scalar file containing the streamline weights

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



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

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


