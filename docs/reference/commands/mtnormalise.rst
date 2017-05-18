.. _mtnormalise:

mtnormalise
===================

Synopsis
--------

Multi-tissue normalise

Usage
--------

::

    mtnormalise [ options ]  input output [ input output ... ]

-  *input output*: list of all input and output tissue compartment files. See example usage in the description. Note that any number of tissues can be normalised

Description
-----------

Globally normalise multiple tissue compartments (e.g. WM, GM, CSF) from multi-tissue CSD such that their sum (of their DC terms) within each voxel is as close to a scalar as possible (Default: sqrt(1/(4*pi)). Normalisation is performed by solving for a single scale factor to adjust each tissue type.

Example usage: mtnormalise wm.mif wm_norm.mif gm.mif gm_norm.mif csf.mif csf_norm.mif

Options
-------

-  **-mask image** define the mask to compute the normalisation within. If not supplied this is estimated automatically

-  **-value number** specify the value to which the summed tissue compartments will be to (Default: sqrt(1/(4*pi) = 0.282)

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

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


