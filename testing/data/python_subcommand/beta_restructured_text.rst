.. _testing_python_subcommand_beta:

testing_python_subcommand beta
==============================

Synopsis
--------

Second sub-interface; demonstrates the require_at_least_one and all_or_none constraints

Usage
-----

::

    testing_python_subcommand beta  [ options ]


Options
-------

-  **-beta_flag** A flag specific to the beta sub-interface

Grouped common options demonstrating hierarchy
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-mode_common** An option located directly within the common parent group

Mutually exclusive common modes
"""""""""""""""""""""""""""""""

-  **-mutex_a** The first mutually-exclusive common mode

-  **-mutex_b** The second mutually-exclusive common mode

*(these options are mutually exclusive; at most one may be specified)*


Cross-group set, first member
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-cross_a** The first member of the cross-group mutual-exclusion set

Cross-group set, second member
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-cross_b** The second member of the cross-group mutual-exclusion set

At-least-one options
^^^^^^^^^^^^^^^^^^^^

-  **-atleast_a** The first at-least-one member

-  **-atleast_b** The second at-least-one member

*(at least one of these options must be specified)*


All-or-none options
^^^^^^^^^^^^^^^^^^^

-  **-both_a** The first all-or-none member

-  **-both_b** The second all-or-none member

*(these options must be specified together or not at all)*


*(the options -cross_a, -cross_b are mutually exclusive; at most one may be specified)*


Standard options
^^^^^^^^^^^^^^^^

-  **-force** force overwrite of output files.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading). (minimum: 0)

-  **-config key value**  *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

Verbosity options
"""""""""""""""""

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages & debug input data.

*(these options are mutually exclusive; at most one may be specified)*


Additional standard options for Python scripts
""""""""""""""""""""""""""""""""""""""""""""""

-  **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

-  **-scratch /path/to/scratch/** manually specify an existing directory in which to generate the scratch directory.

-  **-continue ScratchDir LastFile** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

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

