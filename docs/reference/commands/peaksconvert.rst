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

Under default operation with no command-line options specified, the output image will be identical to the input image, as the MRtrix convention (3-vectors defined with respect to RAS scanner space axes) will be assumed to apply to both cases. This behaviour is only modulated by explicitly providing command-line options that give additional information about the format or reference of either input or output images.

For -in_format and -out_format options, the choices are: - "unitspherical": Each orientation is represented using 2 sequential volumes encoded as azimuth and inclination angles in radians; - "spherical": Each orientation and associated value is represented using 3 sequential volumes, with associated value ("radius") first, followed by aximuth and inclination angles in radians; - "unit3vector": Each orientation is represented using 3 sequential volumes encoded as three dot products with respect to three orthogonal reference axes; - "3vector": Each orientation and associated non-negative value is represented using 3 sequential volumes, with the norm of that 3-vector encoding the associated value and the unit-normalised vector encoding the three dot products with respect to three orthogonal reference axes. The default behaviour throughout MRtrix3 is to interpret data as either "unit3vector" or "3vector" depending upon the context and/or presence of non-unit norm vectors in the data.

For -in_reference and -out_reference options, the choices are: - "xyz": Directions are defined with respect to "real space" / "scanner space", which is independent of the transform stored within the image header, with the assumption that the positive direction of the first axis is that closest to anatomical right, the positive direction of the second axis is that closest to anatomical anterior, and the positive direction of the third axis is that closest to anatomical superior (so-called "RAS+"); - "ijk": Directions are defined with respect to the image axes as represented on the file system; - "fsl": Directions are defined with respect to the internal convention adopted by the FSL software, which is equivalent to "ijk" for images with a negative header transform determinant (so-called "left-handed" coordinate systems) but for images with a positive header transform determinant (which is the case for the "RAS+" convention adopted for both NIfTI and MRtrix3) the interpretation is equivalent to being with respect to the image axes after flipping the first image axis. The default interpretation in MRtrix3, including for this command in the absence of use of one of the command-line options, is "xyz".

Options
-------

Options providing information about the input image
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-in_format choice** specify the format in which the input directions are specified (see Description)

-  **-in_reference choice** specify the reference axes against which the input directions are specified (see Description)

Options providing information about the output image
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-out_format choice** specify the format in which the output directions will be specified (see Description)

-  **-out_reference choice** specify the reference axes against which the output directions will be specified (see Description)

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


