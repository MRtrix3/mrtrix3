.. _for_each:

for_each
========

Synopsis
--------

Perform some arbitrary processing step for each of a set of inputs

Usage
-----

::

    for_each inputs colon command [ options ]

-  *inputs*: Each of the inputs for which processing should be run
-  *colon*: Colon symbol (":") delimiting the for_each inputs & command-line options from the actual command to be executed
-  *command*: The command string to run for each input, containing any number of substitutions listed in the Description section

Description
-----------

This script greatly simplifies various forms of batch processing by enabling the execution of a command (or set of commands) independently for each of a set of inputs. Part of the way that this is achieved is by providing basic text substitutions, which simplify the formation of valid command strings based on the unique components of the input strings on which the script is instructed to execute. The available substitutions are listed below (note that the -test command-line option can be used to ensure correct command string formation prior to actually executing the commands):

   - IN:   The full matching pattern, including leading folders. For example, if the target list contains a file "folder/image.mif", any occurrence of "IN" will be substituted with "folder/image.mif".

   - NAME: The basename of the matching pattern. For example, if the target list contains a file "folder/image.mif", any occurrence of "NAME" will be substituted with "image.mif".

   - PRE:  The prefix of the input pattern (the basename stripped of its extension). For example, if the target list contains a file "folder/my.image.mif.gz", any occurrence of "PRE" will be substituted with "my.image".

   - UNI:  The unique part of the input after removing any common prefix and common suffix. For example, if the target list contains files: "folder/001dwi.mif", "folder/002dwi.mif", "folder/003dwi.mif", any occurrence of "UNI" will be substituted with "001", "002", "003".

Note that due to a limitation of the Python "argparse" module, any command-line OPTIONS that the user intends to provide specifically to the for_each script must appear BEFORE providing the list of inputs on which for_each is intended to operate. While command-line options provided as such will be interpreted specifically by the for_each script, any command-line options that are provided AFTER the COLON separator will form part of the executed COMMAND, and will therefore be interpreted as command-line options having been provided to that underlying command.

Example usages
--------------

-   *Demonstration of basic usage syntax*::

        $ for_each folder/*.mif : mrinfo IN

    This will run the "mrinfo" command for every .mif file present in "folder/". Note that the compulsory colon symbol is used to separate the list of items on which for_each is being instructed to operate, from the command that is intended to be run for each input.

-   *Multi-threaded use of for_each*::

        $ for_each -nthreads 4 freesurfer/subjects/* : recon-all -subjid NAME -all

    In this example, for_each is instructed to run the FreeSurfer command 'recon-all' for all subjects within the 'subjects' directory, with four subjects being processed in parallel at any one time. Whenever processing of one subject is completed, processing for a new unprocessed subject will commence. This technique is useful for improving the efficiency of running single-threaded commands on multi-core systems, as long as the system possesses enough memory to support such parallel processing. Note that in the case of multi-threaded commands (which includes many MRtrix3 commands), it is generally preferable to permit multi-threaded execution of the command on a single input at a time, rather than processing multiple inputs in parallel.

-   *Excluding specific inputs from execution*::

        $ for_each *.nii -exclude 001.nii : mrconvert IN PRE.mif

    Particularly when a wildcard is used to define the list of inputs for for_each, it is possible in some instances that this list will include one or more strings for which execution should in fact not be performed; for instance, if a command has already been executed for one or more files, and then for_each is being used to execute the same command for all other files. In this case, the -exclude option can be used to effectively remove an item from the list of inputs that would otherwise be included due to the use of a wildcard (and can be used more than once to exclude more than one string). In this particular example, mrconvert is instructed to perform conversions from NIfTI to MRtrix image formats, for all except the first image in the directory. Note that any usages of this option must appear AFTER the list of inputs. Note also that the argument following the -exclude option can alternatively be a regular expression, in which case any inputs for which a match to the expression is found will be excluded from processing.

-   *Testing the command string substitution*::

        $ for_each -test * : mrconvert IN PRE.mif

    By specifying the -test option, the script will print to the terminal the results of text substitutions for all of the specified inputs, but will not actually execute those commands. It can therefore be used to verify that the script is receiving the intended set of inputs, and that the text substitutions on those inputs lead to the intended command strings.

Options
-------

- **-exclude "regex"**  *(multiple uses permitted)* Exclude one specific input string / all strings matching a regular expression from being processed (see Example Usage)

- **-test** Test the operation of the for_each script, by printing the command strings following string substitution but not actually executing them

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



**Author:** Robert E. Smith (robert.smith@florey.edu.au) and David Raffelt (david.raffelt@florey.edu.au)

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

