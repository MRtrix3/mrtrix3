.. _tcksample:

tcksample
===================

Synopsis
--------

Sample values of an associated image along tracks

Usage
--------

::

    tcksample [ options ]  tracks image values

-  *tracks*: the input track file
-  *image*: the image to be sampled
-  *values*: the output sampled values

Description
-----------

By default, the value of the underlying image at each point along the track is written to either an ASCII file (with all values for each track on the same line), or a track scalar file (.tsf). Alternatively, some statistic can be taken from the values along each streamline and written to a vector file.

Options
-------

-  **-stat_tck statistic** compute some statistic from the values along each streamline (options are: mean,median,min,max)

-  **-nointerp** do not use trilinear interpolation when sampling image values

-  **-precise** use the precise mechanism for mapping streamlines to voxels (obviates the need for trilinear interpolation) (only applicable if some per-streamline statistic is requested)

-  **-use_tdi_fraction** each streamline is assigned a fraction of the image intensity in each voxel based on the fraction of the track density contributed by that streamline (this is only appropriate for processing a whole-brain tractogram, and images for which the quantiative parameter is additive)

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

* If using -precise option: Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. SIFT: Spherical-deconvolution informed filtering of tractograms. NeuroImage, 2013, 67, 298-312

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


