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

By default, the value of the underlying image at each point along the track is written to either an ASCII file (with all values for each track on the same line), or a track scalar file (.tsf). Alternatively, some statistic can be taken from the values along each streamline and written to a vector file, which can either be in the NumPy .npy format or a numerical text file.

In the circumstance where a per-streamline statistic is requested, the input image can be 4D rather than 3D; in that circumstance, each volume will be sampled independently, and the output (whether in .npy or text format) will be a matrix, with one row per streamline and one column per metric.

If the input image is 4D, and the number of volumes corresponds to an antipodally symmetric spherical harmonics function, then the -sh option must be specified, indicating whether the input image should be interpreted as such a function or whether the input volumes should be sampled individually.

Options
-------

-  **-stat_tck statistic** compute some statistic from the values along each streamline (options are: mean,median,min,max)

-  **-nointerp** do not use trilinear interpolation when sampling image values

-  **-precise** use the precise mechanism for mapping streamlines to voxels (obviates the need for trilinear interpolation) (only applicable if some per-streamline statistic is requested)

-  **-use_tdi_fraction** each streamline is assigned a fraction of the image intensity in each voxel based on the fraction of the track density contributed by that streamline (this is only appropriate for processing a whole-brain tractogram, and images for which the quantiative parameter is additive)

-  **-sh value** Interpret a 4D image input as representing coefficients of a spherical harmonic function, and sample the amplitudes of that function along the streamline

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

* If using -precise option: Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. SIFT: Spherical-deconvolution informed filtering of tractograms. NeuroImage, 2013, 67, 298-312

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2022 the MRtrix3 contributors.

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


