.. _mrcolour:

mrcolour
===================

Synopsis
--------

Apply a colour map to an image

Usage
--------

::

    mrcolour [ options ]  input map output

-  *input*: the input image
-  *map*: the colourmap to apply; choices are: gray,hot,cool,jet,inferno,viridis,pet,colour,rgb
-  *output*: the output image

Description
-----------

Under typical usage, this command will receive as input ad 3D greyscale image, and output a 4D image with 3 volumes corresponding to red-green-blue components; other use cases are possible, and are described in more detail below.

By default, the command will automatically determine the maximum and minimum intensities of the input image, and use that information to set the upper and lower bounds of the applied colourmap. This behaviour can be overridden by manually specifying these bounds using the -upper and -lower options respectively.

Options
-------

-  **-upper value** manually set the upper intensity of the colour mapping

-  **-lower value** manually set the lower intensity of the colour mapping

-  **-colour values** set the target colour for use of the 'colour' map (three comma-separated floating-point values)

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status; alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files (caution: using the same file as input and output might cause unexpected behaviour).

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-config key value** *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2023 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Covered Software is provided under this License on an "as is"
basis, without warranty of any kind, either expressed, implied, or
statutory, including, without limitation, warranties that the
Covered Software is free of defects, merchantable, fit for a
particular purpose or non-infringing.
See the Mozilla Public License v. 2.0 for more details.

For more details, see http://www.mrtrix.org/.


