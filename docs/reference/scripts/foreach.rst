.. _foreach:

foreach
=======

Synopsis
--------

Perform some arbitrary processing step for each of a set of inputs

Usage
--------

::

    foreach inputs command [ options ]

-  *inputs*: Each of the inputs for which processing should be run
-  *command*: The command string to run for each input, containing the substitutions listed in the Description section

Description
-----------

EXAMPLE USAGE: 
  $ foreach folder/*.mif "mrinfo IN"   
  will run mrinfo for each .mif file in "folder"

AVAILABLE SUBSTITUTIONS: 
  IN:   The full matching pattern, including leading folders. For example, if the target list contains a file "folder/image.mif", any occurrence of "IN" will be substituted with "folder/image.mif".  NAME: The basename of the matching pattern. For example, if the target list contains a file "folder/image.mif", any occurrence of "NAME" will be substituted with "image.mif".  PRE:  The prefix of the basename. For example, if the target list contains a file "folder/image.mif", any occurrence of "PRE" will be substituted with "image".  UNI:  The unique prefix of the basename after removing the common suffix. For example, if the target list contains files: "folder/001dwi.mif", "folder/002dwi.mif", "folder/003dwi.mif", any occurrence of "UNI" will be substituted with "001", "002", "003".

Options
-------

Standard options
^^^^^^^^^^^^^^^^

- **-continue <TempDir> <LastFile>** Continue the script from a previous execution; must provide the temporary directory path, and the name of the last successfully-generated file

- **-force** Force overwrite of output files if pre-existing

- **-help** Display help information for the script

- **-nocleanup** Do not delete temporary files during script, or temporary directory at script completion

- **-nthreads number** Use this number of threads in MRtrix multi-threaded applications (0 disables multi-threading)

- **-tempdir /path/to/tmp/** Manually specify the path in which to generate the temporary directory

- **-quiet** Suppress all console output during script execution

- **-info** Display additional information and progress for every command invoked

- **-debug** Display additional debugging information over and above the output of -info

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

