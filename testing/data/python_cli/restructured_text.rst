.. _testing_python_cli:

testing_python_cli
==================

Synopsis
--------

Test operation of the Python command-line interface

Usage
-----

::

    testing_python_cli  [ options ]


Options
-------

Custom types
^^^^^^^^^^^^

- **-bool value** A boolean input

- **-int_unbound value** An integer; unbounded

- **-int_nonnegative value** An integer; non-negative

- **-int_bounded value** An integer; bound range

- **-float_unbound value** A floating-point; unbounded

- **-float_nonneg value** A floating-point; non-negative

- **-float_bounded value** A floating-point; bound range

- **-int_seq values** A comma-separated list of integers

- **-float_seq values** A comma-separated list of floating-points

- **-dir_in directory** An input directory

- **-dir_out directory** An output directory

- **-file_in file** An input file

- **-file_out file** An output file

- **-image_in image** An input image

- **-image_out image** An output image

- **-tracks_in trackfile** An input tractogram

- **-tracks_out trackfile** An output tractogram

- **-various spec** An option that accepts various types of content

Complex interfaces; nargs, metavar, etc.
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nargs_plus string <space-separated list of additional strings>** A command-line option with nargs="+", no metavar

- **-nargs_asterisk <space-separated list of strings>** A command-line option with nargs="*", no metavar

- **-nargs_question <optional string>** A command-line option with nargs="?", no metavar

- **-nargs_two string string** A command-line option with nargs=2, no metavar

- **-metavar_one metavar** A command-line option with nargs=1 and metavar="metavar"

- **-metavar_two metavar metavar** A command-line option with nargs=2 and metavar="metavar"

- **-metavar_tuple metavar_one metavar_two** A command-line option with nargs=2 and metavar=("metavar_one", "metavar_two")

- **-append string**  *(multiple uses permitted)* A command-line option with "append" action (can be specified multiple times)

Built-in types
^^^^^^^^^^^^^^

- **-flag** A binary flag

- **-string_implicit string** A built-in string (implicit)

- **-string_explicit str** A built-in string (explicit)

- **-choice choice** A selection of choices; one of: One, Two, Three

- **-int_builtin int** An integer; built-in type

- **-float_builtin float** A floating-point; built-in type

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify an existing directory in which to generate the scratch directory.

- **-continue ScratchDir LastFile** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

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

