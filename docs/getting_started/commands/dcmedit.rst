dcmedit
===========

Synopsis
--------

::

    dcmedit [ options ]  file

-  *file*: the DICOM file to be edited.

Description
-----------

Edit DICOM file in-place. Note that this simply replaces the existing
values without modifying the DICOM structure in any way. Replacement
text will be truncated if it is too long to fit inside the existing tag.

WARNING: this command will modify existing data! It is recommended to
run this command on a copy of the original data set to avoid loss of
data.

Options
-------

-  **-anonymise** remove any identifiable information, by replacing the
   following tags: - any tag with Value Representation PN will be
   replaced with 'anonymous' - tag (0010,0030) PatientBirthDate will be
   replaced with an empty stringWARNING: there is no guarantee that this
   command will remove all identiable information, since such
   information may be contained in any number of private vendor-specific
   tags. You will need to double-check the results independently if you
   need to ensure anonymity.

-  **-id text** replace all ID tags with string supplied. This consists
   of tags (0010, 0020) PatientID and (0010, 1000) OtherPatientIDs

-  **-tag group element newvalue** replace specific tag.

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
