.. _tckdecimatecalibrate:

tckdecimatecalibrate
===================

Synopsis
--------

Calibrate a streamline decimation resampler against geometric error and computational cost

Usage
--------

::

    tckdecimatecalibrate [ options ]  in_tracks values

-  *in_tracks*: the input track file
-  *values*: the set of decimation parameter values to calibrate over (comma-separated list and/or a min:step:max range); interpreted as the fast decimator density knob mu by default, or as the slow decimator Hausdorff-distance tolerance (mm) under -algorithm slow

Description
-----------

This command evaluates one of the two streamline decimation resamplers exposed by tckresample: the curvature-adaptive single-pass "fast" decimator (the -decimate_fast option), governed by a dimensionless density knob mu (output vertices per unit curvature-weighted arc length; larger values retain more vertices); and the greedy knot-insertion "slow" decimator (the -decimate_slow option), governed directly by a deviation tolerance in mm (smaller values retain more vertices). Use the -algorithm option to select which resampler to calibrate (default: fast).

For every streamline in the input tractogram and every value in the swept parameter set, the selected decimator is run and the symmetric Hausdorff distance (in mm) between the original and decimated tension-Catmull-Rom splines is measured. Per parameter value the command reports percentiles [50, 75, 95, 99, 99.9, 100] of that per-streamline distance distribution, the mean output/input vertex ratio (compression), the mean absolute output vertex count, and the total time spent inside the decimator (summed across all streamlines; the resampling cost only, excluding the Hausdorff measurement).

For the fast algorithm the swept values are mu, and the Hausdorff percentiles expose the geometric error that the chosen density incurs. For the slow algorithm the swept values are the deviation tolerances themselves; the reconstruction is bounded within the tolerance by construction, so the headline output is the compression ratio achieved at each tolerance, while the reported Hausdorff percentiles verify that the spline-level error respects the bound.

The parameter set may be specified either as a comma-separated list of explicit values (e.g. "1.0,2.0,4.0") or as a min:step:max range (e.g. "1.0:1.0:5.0"); every value must be strictly positive.

Options
-------

-  **-algorithm name** the decimation resampler to calibrate; one of fast, slow (default: fast)

-  **-csv path** write the calibration table to a CSV file

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


