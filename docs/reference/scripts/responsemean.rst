.. _responsemean:

responsemean
============

Synopsis
--------

Calculate the mean response function from a set of text files

Usage
--------

::

    responsemean inputs output [ options ]

-  *inputs*: The input response functions
-  *output*: The output mean response function

Description
-----------

Example usage: /c/Users/rob/mrtrix3/bin/responsemean input_response1.txt input_response2.txt input_response3.txt ... output_average_response.txt

All response function files provided must contain the same number of unique b-values (lines), as well as the same number of coefficients per line.

As long as the number of unique b-values is identical across all input files, the coefficients will be averaged. This is performed on the assumption that the actual acquired b-values are identical. This is however impossible for the responsemean command to determine based on the data provided; it is therefore up to the user to ensure that this requirement is satisfied.

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

optional arguments
^^^^^^^^^^^^^^^^^^

- **-legacy** Use the legacy behaviour of former command 'average_response': average response function coefficients directly, without compensating for global magnitude differences between input files

--------------



**Author:** David Raffelt (david.raffelt@florey.edu.au) and Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2018 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/

MRtrix3 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/

