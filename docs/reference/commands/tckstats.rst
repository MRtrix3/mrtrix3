.. _tckstats:

tckstats
===================

Synopsis
--------

Calculate statistics on streamlines lengths

Usage
--------

::

    tckstats [ options ]  tracks_in

-  *tracks_in*: the input track file

Description
-----------

Where a command-line argument accepts tractogram sidecar data (such as streamline weights), it may be given as: "<path>" to read from / write to a standalone external file; "<path>::<field>" to access a named field of sidecar data embedded within the specified tractogram dataset; or "::<field>" to access a named field of sidecar data embedded within the command's own input or output tractogram (the only form able to reference embedded sidecar data of a piped tractogram, which has no command-line filesystem path).

Options
-------

-  **-output field** *(multiple uses permitted)* output only the field specified. Multiple such options can be supplied if required. Choices are: mean, median, std, min, max, count. Useful for use in scripts.

-  **-histogram path** output a histogram of streamline lengths

-  **-dump path** dump the streamlines lengths to a text file

-  **-ignorezero** do not generate a warning if the track file contains streamlines with zero length

-  **-tck_weights_in spec** specify the streamline weights: either a standalone scalar file, or "[<tractogram>]::<field>" naming a per-streamline field of the input tractogram (an empty <tractogram>, i.e. "::<field>", refers to the command's own input tractogram, which is the only way to name a field of a piped input)

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


