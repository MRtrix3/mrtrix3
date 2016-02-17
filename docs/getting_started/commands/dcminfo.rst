dcminfo
===========

Synopsis
--------

::

    dcminfo [ options ]  file

-  *file*: the DICOM file to be scanned.

Description
-----------

output DICOM fields in human-readable format.

Options
-------

-  **-all** print all DICOM fields.

-  **-csa** print all Siemens CSA fields

-  **-tag group element** print field specified by the group & element
   tags supplied. Tags should be supplied as Hexadecimal (i.e. as they
   appear in the -all listing).

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
