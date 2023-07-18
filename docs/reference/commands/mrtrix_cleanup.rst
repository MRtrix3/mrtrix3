.. _mrtrix_cleanup:

mrtrix_cleanup
==============

Synopsis
--------

Clean up residual temporary files & scratch directories from MRtrix3 commands

Usage
-----

::

    mrtrix_cleanup path [ options ]

-  *path*: Path from which to commence filesystem search

Description
-----------

This script will search the file system at the specified location (and in sub-directories thereof) for any temporary files or directories that have been left behind by failed or terminated MRtrix3 commands, and attempt to delete them.

Note that the script's search for temporary items will not extend beyond the user-specified filesystem location. This means that any built-in or user-specified default location for MRtrix3 piped data and scripts will not be automatically searched. Cleanup of such locations should instead be performed explicitly: e.g. "mrtrix_cleanup /tmp/" to remove residual piped images from /tmp/.

This script should not be run while other MRtrix3 commands are being executed: it may delete temporary items during operation that may lead to unexpected behaviour.

Options
-------

- **-test** Run script in test mode: will list identified files / directories, but not attempt to delete them

- **-failed file** Write list of items that the script failed to delete to a text file

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify the path in which to generate the scratch directory.

- **-continue <ScratchDir> <LastFile>** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

Standard options
^^^^^^^^^^^^^^^^

- **-info** display information messages.

- **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

- **-debug** display debugging messages.

- **-force** force overwrite of output files.

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

- **-config key value**  *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

- **-help** display this information page and exit.

- **-version** display version information and exit.

References
^^^^^^^^^^

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2023 the MRtrix3 contributors.

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

