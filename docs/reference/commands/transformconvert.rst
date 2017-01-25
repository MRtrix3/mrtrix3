.. _transformconvert:

transformconvert
===========

Synopsis
--------

::

    transformconvert [ options ]  input [ input ... ] operation output

-  *input*: the input for the specified operation
-  *operation*: the operation to perform, one of:flirt_import, itk_import.flirt_import: Convert a transformation matrix produced by FSL's flirt command into a format usable by MRtrix. You'll need to provide as additional arguments the NIfTI images that were passed to flirt with the -in and -ref options:matrix_in in ref flirt_import outputitk_import: Convert a plain text transformation matrix file produced by ITK's (ANTS, Slicer) affine registration into a format usable by MRtrix.
-  *output*: the output transformation matrix.

Description
-----------

This command's function is to convert linear transformation matrices.

It allows to convert the transformation matrix provided by FSL's flirt command 

and ITK's linear transformation format to a format usable in MRtrix.

Options
-------

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



**Author:** Max Pietsch (maximilian.pietsch@kcl.ac.uk)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.

