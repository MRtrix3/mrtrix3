.. _foreach:

foreach
=======

Synopsis
--------

Perform some arbitrary processing step for each of a set of inputs

Usage
--------

::

    foreach inputs colon command [ options ]

-  *inputs*: Each of the inputs for which processing should be run
-  *colon*: Colon symbol (":") delimiting the foreach inputs & command-line options from the actual command to be executed
-  *command*: The command string to run for each input, containing any number of substitutions listed in the Description section

Description
-----------

EXAMPLE USAGE: 
  $ foreach folder/*.mif : mrinfo IN   
  will run mrinfo for each .mif file in "folder"

AVAILABLE SUBSTITUTIONS: 
  IN:   The full matching pattern, including leading folders. For example, if the target list contains a file "folder/image.mif", any occurrence of "IN" will be substituted with "folder/image.mif".  NAME: The basename of the matching pattern. For example, if the target list contains a file "folder/image.mif", any occurrence of "NAME" will be substituted with "image.mif".  PRE:  The prefix of the basename. For example, if the target list contains a file "folder/image.mif", any occurrence of "PRE" will be substituted with "image".  UNI:  The unique part of the input after removing any common prefix and common suffix. For example, if the target list contains files: "folder/001dwi.mif", "folder/002dwi.mif", "folder/003dwi.mif", any occurrence of "UNI" will be substituted with "001", "002", "003".

Note that due to a limitation of the Python argparse module, any command-line options provided to the foreach script must appear before any inputs are specified.

Such command-line options provided before the list of inputs and colon separator will be interpreted by the foreach script; any command-line options provided after this colon will form part of the input to the executed command.

Options
-------

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete temporary files during script execution, and do not delete temporary directory at script completion

- **-tempdir /path/to/tmp/** manually specify the path in which to generate the temporary directory

- **-continue <TempDir> <LastFile>** continue the script from a previous execution; must provide the temporary directory path, and the name of the last successfully-generated file

Standard options
^^^^^^^^^^^^^^^^

- **-info** display information messages.

- **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

- **-debug** display debugging messages.

- **-force** force overwrite of output files.

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

- **-help** display this information page and exit.

- **-version** display version information and exit.

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au) and David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (c) 2008-2018 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/

MRtrix3 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/

