.. _fixelcorrespondence:

fixelcorrespondence
===========

Synopsis
--------

::

    fixelcorrespondence [ options ]  subject_data template_folder output_folder output_data

-  *subject_data*: the input subject fixel data file. This should be a file inside the fixel folder
-  *template_folder*: the input template fixel folder.
-  *output_folder*: the output fixel folder.
-  *output_data*: the name of the output fixel data file. This will be placed in the output fixel folder

Description
-----------

Obtain fixel-fixel correpondence between a subject fixel image and a template fixel mask.It is assumed that the subject image has already been spatially normalised and is aligned with the template. The output fixel image will have the same fixels (and directions) of the template.

Options
-------

-  **-angle value** the max angle threshold for computing inter-subject fixel correspondence (Default: 45 degrees)

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



**Author:** David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

