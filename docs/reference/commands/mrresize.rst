.. _mrresize:

mrresize
===================

Synopsis
--------

Resize an image by defining the new image resolution, voxel size or a scale factor

Usage
--------

::

    mrresize [ options ]  input output

-  *input*: input image to be resized.
-  *output*: the output image.

Description
-----------

Note that if the image is 4D, then only the first 3 dimensions can be resized.

Also note that if the image is down-sampled, the appropriate smoothing is automatically applied using Gaussian smoothing.

Options
-------

-  **-size dims** define the new image size for the output image. This should be specified as a comma-separated list.

-  **-voxel size** define the new voxel size for the output image. This can be specified either as a single value to be used for all dimensions, or as a comma-separated list of the size for each voxel dimension.

-  **-scale factor** scale the image resolution by the supplied factor. This can be specified either as a single value to be used for all dimensions, or as a comma-separated list of scale factors for each dimension.

-  **-interp method** set the interpolation method to use when resizing (choices: nearest, linear, cubic, sinc. Default: cubic).

Data type options
^^^^^^^^^^^^^^^^^

-  **-datatype spec** specify output image data type. Valid choices are: float32, float32le, float32be, float64, float64le, float64be, int64, uint64, int64le, uint64le, int64be, uint64be, int32, uint32, int32le, uint32le, int32be, uint32be, int16, uint16, int16le, uint16le, int16be, uint16be, cfloat32, cfloat32le, cfloat32be, cfloat64, cfloat64le, cfloat64be, int8, uint8, bit.

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

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.

