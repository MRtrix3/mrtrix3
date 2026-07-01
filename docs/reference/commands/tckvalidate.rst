.. _tckvalidate:

tckvalidate
===================

Synopsis
--------

Validate a tractogram file and its associated sidecar data

Usage
--------

::

    tckvalidate [ options ]  tracks_in

-  *tracks_in*: the input tractogram file

Description
-----------

This command checks that a tractogram file is well-formed. For the in-house ".tck" format, the binary data section consists of a sequence of 3-float triplets, each of which must be exactly one of: a regular vertex (all three components finite), an inter-streamline delimiter (all three components NaN), or the mandatory end-of-file barrier (all three components infinity); the end-of-file barrier must be the last triplet in the file.

The following hard violations cause the command to fail for a ".tck" file: (1) a triplet that is partially non-finite (i.e. not all-finite, not all-NaN, and not all-infinity); (2) any data present after the end-of-file barrier; (3) the end of the binary data section is reached without an end-of-file barrier (truncated file); (4) the last streamline body is not terminated by a NaN delimiter before the end-of-file barrier; (5) the "count" field is absent from the file header; (6) the "count" field in the file header does not match the number of streamlines actually present in the file.

For any supported tractography format, the entire tractogram is read and the "count" field in the header (where present) is checked against the number of streamlines actually read.

Associated sidecar data can additionally be validated against the tractogram using the -tsf and -tck_weights options; every per-streamline (data-per-streamline) field must contain exactly one entry per streamline, and every per-vertex (data-per-vertex) field must contain one scalar sequence per streamline whose length matches that streamline's vertex count.

The command also reports the presence of streamlines with zero vertices or exactly one vertex, which are degenerate cases that may indicate issues with the tractography algorithm that produced the file.

Options
-------

-  **-tsf path** *(multiple uses permitted)* validate a per-vertex (data-per-vertex) track scalar file (.tsf) against the tractogram (may be specified multiple times)

-  **-tck_weights path** *(multiple uses permitted)* validate a per-streamline (data-per-streamline) numerical sidecar file (plain-text / .csv / .npy) against the tractogram (may be specified multiple times)

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


