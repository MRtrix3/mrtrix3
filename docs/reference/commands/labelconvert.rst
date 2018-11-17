.. _labelconvert:

labelconvert
===================

Synopsis
--------

Convert a connectome node image from one lookup table to another

Usage
--------

::

    labelconvert [ options ]  path_in lut_in lut_out image_out

-  *path_in*: the input image
-  *lut_in*: the connectome lookup table corresponding to the input image
-  *lut_out*: the target connectome lookup table for the output image
-  *image_out*: the output image

Description
-----------

Typical usage is to convert a parcellation image provided by some other software, based on the lookup table provided by that software, to conform to a new lookup table, particularly one where the node indices increment from 1, in preparation for connectome construction; examples of such target lookup table files are provided in share//mrtrix3//labelconvert//, but can be created by the user to provide the desired node set // ordering // colours.

Example usages
--------------

-   *Convert a Desikan-Killiany parcellation image as provided by FreeSurfer to have nodes incrementing from 1*::

        $ labelconvert aparc+aseg.mgz FreeSurferColorLUT.txt mrtrix3//share//mrtrix3//labelconvert//fs_default.txt nodes.mif

    Paths to the files in the example above would need to be revised according to their locations on the user's system.

Options
-------

-  **-spine image** provide a manually-defined segmentation of the base of the spine where the streamlines terminate, so that this can become a node in the connection matrix.

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



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2018 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/

MRtrix3 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/


