.. _mrmodelfield:

mrmodelfield
===================

Synopsis
--------

Model an input image using low frequency 3D polynomial basis functions

Usage
--------

::

    mrmodelfield [ options ]  input output

-  *input*: the input image
-  *output*: the output image representing the fit

Description
-----------

This command was designed to estimate a DWI bias field using the sum of normalised multi-tissue CSD compartments.

Options
-------

-  **-mask image** use only voxels within the supplied mask for the model fit. If not supplied this command will compute a mask

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



**Author:** David Raffelt (david.raffelt@florey.edu.au) & Rami Tabbara (rami.tabbara@florey.edu.au)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


