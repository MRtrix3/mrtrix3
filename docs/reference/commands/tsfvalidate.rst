.. _tsfvalidate:

tsfvalidate
===================

Synopsis
--------

Validate a track scalar file against its corresponding tractogram

Usage
--------

::

    tsfvalidate [ options ]  tsf tracks

-  *tsf*: the input track scalar file
-  *tracks*: the tractogram from which the track scalar file was derived

Description
-----------

This command checks that a track scalar file (.tsf) is consistent with the tractogram (.tck) from which it was derived. The following conditions are verified:

1. Both files must contain a "timestamp" field in their headers, and the two timestamps must be identical. The timestamp is copied from the tractogram into the scalar file when the scalar file is created, so a mismatch indicates that the scalar file was not produced from the supplied tractogram.

2. The track scalar file header must contain a "count" field.

3. The value of the "count" field in the track scalar file header must match the number of scalar sequences actually present in the file.

4. The number of scalar sequences in the track scalar file must equal the number of streamlines in the tractogram.

5. For every streamline, the number of scalar values in the corresponding scalar sequence must equal the number of vertices in the streamline.

Options
-------

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


