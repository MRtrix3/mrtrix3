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

Currently, this command's only function is to convert the transformation matrix provided by FSL's flirt command to a format usable in MRtrix.

Options
-------

-  **-flirt_import in ref** convert a transformation matrix produced by FSL's flirt command into a format usable by MRtrix. You'll need to provide as additional arguments the save NIfTI images that were passed to flirt with the -in and -ref options.

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications

-  **-failonwarn** terminate program if a warning is produced

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------



**Author:** J-Donald Tournier (jdtournier@gmail.com)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

