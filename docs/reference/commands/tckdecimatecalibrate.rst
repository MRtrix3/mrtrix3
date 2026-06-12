.. _tckdecimatecalibrate:

tckdecimatecalibrate
===================

Synopsis
--------

Calibrate the fast streamline decimation density parameter against geometric error

Usage
--------

::

    tckdecimatecalibrate [ options ]  in_tracks mu

-  *in_tracks*: the input track file
-  *mu*: the set of density values to calibrate over (comma-separated list and/or a min:step:max range)

Description
-----------

The fast decimator exposed by the tckresample -decimate_fast option is governed by a single dimensionless density knob (mu): the number of output vertices per unit curvature-weighted arc length. This command sweeps mu over a user-specified range and, for each value, decimates every streamline in the input tractogram and measures the symmetric Hausdorff distance (in mm) between the original and decimated tension-Catmull-Rom splines.

For each mu it reports percentiles [50, 75, 95, 99, 99.9, 100] of the distribution of those per-streamline Hausdorff distances, allowing a value of mu to be selected that bounds the geometric error introduced by decimation to within a tolerance appropriate for the data. The mean output/input vertex ratio (compression) is also reported per mu to expose the fidelity-versus-size trade-off.

The mu range may be specified either as a comma-separated list of explicit values (e.g. "1.0,2.0,4.0") or as a min:step:max range (e.g. "1.0:1.0:5.0"); every value must be strictly positive.

Options
-------

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


