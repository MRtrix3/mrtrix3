.. _fixelcorrespondence:

fixelcorrespondence
===================

Synopsis
--------

Obtain fixel-fixel correpondence between a subject fixel image and a template fixel mask

Usage
--------

::

    fixelcorrespondence [ options ]  subject_data template_directory output_directory output_data

-  *subject_data*: the input subject fixel data file. This should be a file inside the fixel directory
-  *template_directory*: the input template fixel directory.
-  *output_directory*: the output fixel directory.
-  *output_data*: the name of the output fixel data file. This will be placed in the output fixel directory

Description
-----------

It is assumed that the subject image has already been spatially normalised and is aligned with the template. The output fixel image will have the same fixels (and directions) of the template.

Options
-------

-  **-angle value** the max angle threshold for computing inter-subject fixel correspondence (Default: 45 degrees)

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------



**Author:** David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


