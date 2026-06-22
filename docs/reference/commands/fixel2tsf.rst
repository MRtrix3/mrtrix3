.. _fixel2tsf:

fixel2tsf
===================

Synopsis
--------

Map fixel values to a track scalar file based on an input tractogram

Usage
--------

::

    fixel2tsf [ options ]  fixel_in tracks tsf

-  *fixel_in*: the input fixel data file (within the fixel directory)
-  *tracks*: the input track file
-  *tsf*: the output track scalar file, or a "DATASET::NAME" embedded sidecar field

Description
-----------

This command is useful for visualising all brain fixels (e.g. the output from fixelcfestats) in 3D.

By default the sampled fixel values are written to a standalone track scalar file (.tsf). Alternatively, they may be embedded into a tractography dataset as a named per-vertex (data-per-vertex) sidecar field, using the qualified "DATASET::NAME" form for the output argument. If DATASET does not yet exist it is created as a copy of the input tractogram carrying the new field, generated within the same pass that performs the sampling. If DATASET already exists and its format supports adding a field in place (a TRX directory or uncompressed archive), the field is appended without rewriting the streamline data; the -force option is then required only if a field named NAME is already present. If DATASET already exists but cannot be augmented in place (e.g. ".trk", or a compressed TRX archive), the -force option is required and the dataset is rewritten with the field added.

Fixel data are stored utilising the fixel directory format described in the main documentation, which can be found at the following link:  |br|
https://mrtrix.readthedocs.io/en/3.0.8/fixel_based_analysis/fixel_directory_format.html

Where a command-line argument accepts tractogram sidecar data (such as streamline weights), it may be given as: "<path>" to read from / write to a standalone external file; "<path>::<field>" to access a named field of sidecar data embedded within the specified tractogram dataset; or "::<field>" to access a named field of sidecar data embedded within the command's own input or output tractogram (the only form able to reference embedded sidecar data of a piped tractogram, which has no command-line filesystem path).

Options
-------

-  **-angle value** the max anglular threshold for computing correspondence between a fixel direction and track tangent (default = 45 degrees)

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



**Author:** David Raffelt (david.raffelt@florey.edu.au)

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


