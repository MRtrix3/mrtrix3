Synopsis
--------

Verify operation of the C++ command-line interface & parser

Usage
--------

::

    testing_cpp_cli [ options ] 


Options
-------

-  **-flag** An option flag that takes no arguments

-  **-text spec** a text input

-  **-bool value** a boolean input

-  **-int_unbound value** an integer input (unbounded)

-  **-int_nonneg value** a non-negative integer

-  **-int_bound value** a bound integer

-  **-float_unbound value** a floating-point number (unbounded)

-  **-float_nonneg value** a non-negative floating-point number

-  **-float_bound value** a bound floating-point number

-  **-int_seq values** a comma-separated sequence of integers

-  **-float_seq values** a comma-separated sequence of floating-point numbers

-  **-choice item** a choice from a set of options

-  **-file_in input** an input file

-  **-file_out output** an output file

-  **-dir_in input** an input directory

-  **-dir_out output** an output directory

-  **-tracks_in input** an input tractogram

-  **-tracks_out output** an output tractogram

-  **-any spec** an argument that could accept any of the various forms

-  **-nargs_two first second** A command-line option that accepts two arguments

-  **-multiple spec** *(multiple uses permitted)* A command-line option that can be specified multiple times

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

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2024 the MRtrix3 contributors.

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


