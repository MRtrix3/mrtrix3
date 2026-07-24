.. _warpconvert_warpfull2deformation:

warpconvert warpfull2deformation
================================

Synopsis
--------

Convert a 5D warpfull series to a deformation field

Usage
-----

::

    warpconvert warpfull2deformation [ options ]  in out

-  *in*: the input warpfull image.
-  *out*: the output deformation field image.

Options
-------

-  **-template image** define a template image (the warpfull grid lies in the midway space between image 1 & 2). For example, to generate the deformation field that maps image1 to image2, supply image2 as the template image

-  **-midway_space** output only the non-linear warp mapping an input image to the midway space defined by the warpfull grid. If a linear transform exists in the warpfull file header then it will be composed and included in the output.

-  **-from image** define the direction of the desired output field. Use -from 1 to obtain the image1->image2 field and -from 2 for image2->image1. Can be combined with -midway_space to produce a field that only maps to midway space. (range: 1 to 2)

Data type options
^^^^^^^^^^^^^^^^^

-  **-datatype spec** specify output image data type. (choices: float16, float16le, float16be, float32, float32le, float32be, float64, float64le, float64be, int64, uint64, int64le, uint64le, int64be, uint64be, int32, uint32, int32le, uint32le, int32be, uint32be, int16, uint16, int16le, uint16le, int16be, uint16be, cfloat16, cfloat16le, cfloat16be, cfloat32, cfloat32le, cfloat32be, cfloat64, cfloat64le, cfloat64be, int8, uint8, bit)

Standard options
^^^^^^^^^^^^^^^^

-  **-force** force overwrite of output files (caution: using the same file as input and output might cause unexpected behaviour).

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading). (minimum: 0)

-  **-config key value**  *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

Verbosity options
"""""""""""""""""

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status; alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages & debug input data.

*(these options are mutually exclusive; at most one may be specified)*

References
^^^^^^^^^^

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (c) 2008-2026 the MRtrix3 contributors.

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


