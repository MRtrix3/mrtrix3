## Synopsis

Test operation of the Python command-line interface

## Usage

    testing_python_cli  [ options ]

## Options

#### Built-in types

+ **-flag**<br>A binary flag

+ **-string_implicit str**<br>A built-in string, its type declared through the builtin str

+ **-string_explicit str**<br>A built-in string (explicit)

+ **-choice choice**<br>A selection of choices (choices: One, Two, Three; default: One)

+ **-int_builtin int**<br>An integer; built-in type

+ **-float_builtin float**<br>A floating-point; built-in type

#### Multi-argument and repeatable options

+ **-tuple_names first second**<br>A two-argument option; its fields are identified by their names

+ **-tuple_metavars metavar_one metavar_two**<br>A two-argument option; its fields override their display ids

+ **-tuple_described index value**<br>A two-argument option; its fields carry their own descriptions and types

    - *index*: the index of the item (minimum: 0)
    - *value*: the value to be assigned (range: 0.0 to 1.0)
+ **-metavar_one metavar**<br>A single-argument option with metavar="metavar"

+ **-multiple str**  *(multiple uses permitted)*<br>A command-line option that may be specified multiple times

+ **-unused**<br>An option deliberately left unread to exercise unused-option tracking

#### Custom types

+ **-bool value**<br>A boolean input

+ **-int_unbound value**<br>An integer; unbounded

+ **-int_nonnegative value**<br>An integer; non-negative (minimum: 0)

+ **-int_bounded value**<br>An integer; bound range (range: 0 to 100; default: 50)

+ **-float_unbound value**<br>A floating-point; unbounded

+ **-float_nonneg value**<br>A floating-point; non-negative (minimum: 0.0)

+ **-float_bounded value**<br>A floating-point; bound range (range: 0.0 to 1.0; default: 0.5)

+ **-int_seq values**<br>A comma-separated list of integers

+ **-float_seq values**<br>A comma-separated list of floating-points

+ **-lmax value**<br>A spherical-harmonic degree; non-negative even integer (minimum: 0; must be even)

+ **-lmax_bounded value**<br>A spherical-harmonic degree with an explicit upper bound (range: 0 to 8; must be even)

+ **-lmax_seq values**<br>A comma-separated sequence of spherical-harmonic degrees (values must be non-negative and even)

+ **-dir_in directory**<br>An input directory

+ **-dir_out directory**<br>An output directory

+ **-file_in file**<br>An input file

+ **-file_out file**<br>An output file

+ **-image_in image**<br>An input image

+ **-image_out image**<br>An output image

+ **-tracks_in trackfile**<br>An input tractogram

+ **-tracks_out trackfile**<br>An output tractogram

+ **-custom custom**<br>An option with custom type

#### Standard options

+ **-force**<br>force overwrite of output files.

+ **-nthreads number**<br>use this number of threads in multi-threaded applications (set to 0 to disable multi-threading). (minimum: 0)

+ **-config key value**  *(multiple uses permitted)*<br>temporarily set the value of an MRtrix config file entry.

+ **-help**<br>display this information page and exit.

+ **-version**<br>display version information and exit.

##### Verbosity options

+ **-info**<br>display information messages.

+ **-quiet**<br>do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

+ **-debug**<br>display debugging messages & debug input data.

*(these options are mutually exclusive; at most one may be specified)*

##### Additional standard options for Python scripts

+ **-nocleanup**<br>do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

+ **-scratch /path/to/scratch/**<br>manually specify an existing directory in which to generate the scratch directory.

+ **-continue ScratchDir LastFile**<br>continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

## References

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

---

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

