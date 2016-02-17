transformcalc
===========

Synopsis
--------

::

    transformcalc [ options ]  input output

-  *input*: input transformation matrix
-  *output*: the output transformation matrix.

Description
-----------

edit transformations.

Currently, this command's only function is to convert the transformation
matrix provided by FSL's flirt command to a format usable in MRtrix.

Options
-------

-  **-flirt_import in ref** convert a transformation matrix produced
   by FSL's flirt command into a format usable by MRtrix. You'll need to
   provide as additional arguments the save NIfTI images that were
   passed to flirt with the -in and -ref options.

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same
   file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded
   applications

-  **-failonwarn** terminate program if a warning is produced

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------

MRtrix new_syntax-1436-ge228c30b, built Feb 17 2016

**Author:** J-Donald Tournier (jdtournier@gmail.com)

**Copyright:** Copyright (C) 2008 Brain Research Institute, Melbourne,
Australia. This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.
