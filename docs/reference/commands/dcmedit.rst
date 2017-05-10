.. _dcmedit:

dcmedit
===================

Synopsis
--------

Edit DICOM file in-place

Usage
--------

::

    dcmedit [ options ]  file

-  *file*: the DICOM file to be edited.

Description
-----------

Note that this command simply replaces the existing values without modifying the DICOM structure in any way. Replacement text will be truncated if it is too long to fit inside the existing tag.

WARNING: this command will modify existing data! It is recommended to run this command on a copy of the original data set to avoid loss of data.

Options
-------

-  **-anonymise** remove any identifiable information, by replacing the following tags: - any tag with Value Representation PN will be replaced with 'anonymous' - tag (0010,0030) PatientBirthDate will be replaced with an empty stringWARNING: there is no guarantee that this command will remove all identiable information, since such information may be contained in any number of private vendor-specific tags. You will need to double-check the results independently if you need to ensure anonymity.

-  **-id text** replace all ID tags with string supplied. This consists of tags (0010, 0020) PatientID and (0010, 1000) OtherPatientIDs

-  **-tag group element newvalue** replace specific tag.

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

-  **-failonwarn** terminate program if a warning is produced

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------



**Author:** J-Donald Tournier (jdtournier@gmail.com)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


