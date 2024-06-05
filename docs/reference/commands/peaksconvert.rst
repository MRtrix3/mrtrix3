.. _peaksconvert:

peaksconvert
===================

Synopsis
--------

Convert peak directions images between formats and/or conventions

Usage
--------

::

    peaksconvert [ options ]  input output

-  *input*: the input directions image
-  *output*: the output directions image

Description
-----------

Under default operation with no command-line options specified, the output image will be identical to the input image, as the MRtrix convention (3-vectors defined with respect to RAS scanner space axes) will be assumed to apply to both cases. This behaviour is only modulated by explicitly providing command-line options that give additional information about the format or convention of either image.

Options
-------

Options providing information about the input image
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-in_format choice** specify the format in which the input directions are specified

-  **-in_reference choice** specify the reference axes against which the input directions are specified (assumed to be real / scanner space if omitted)

Options providing information about the output image
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-out_format choice** specify the format in which the output directions will be specified (will default to 3-vectors if omitted)

-  **-out_reference choice** specify the reference axes against which the output directions will be specified (defaults to real / scanner space if omitted)

-  **-fill value** specify value to be inserted into output image in the absence of valid information

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

**Copyright:** Copyright (c) 2008-2024 the MRtrix3 contributors.

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


