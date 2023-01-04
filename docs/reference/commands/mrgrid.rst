.. _mrgrid:

mrgrid
===================

Synopsis
--------

Modify the grid of an image without interpolation (cropping or padding) or by regridding to an image grid with modified orientation, location and or resolution. The image content remains in place in real world coordinates.

Usage
--------

::

    mrgrid [ options ]  input operation output

-  *input*: input image to be regridded.
-  *operation*: the operation to be performed, one of: regrid, crop, pad.
-  *output*: the output image.

Description
-----------

- regrid: This operation performs changes of the voxel grid that require interpolation of the image such as changing the resolution or location and orientation of the voxel grid. If the image is down-sampled, the appropriate smoothing is automatically applied using Gaussian smoothing unless nearest neighbour interpolation is selected or oversample is changed explicitly. The resolution can only be changed for spatial dimensions. 

- crop: The image extent after cropping, can be specified either manually for each axis dimensions, or via a mask or reference image. The image can be cropped to the extent of a mask. This is useful for axially-acquired brain images, where the image size can be reduced by a factor of 2 by removing the empty space on either side of the brain. Note that cropping does not extend the image beyond the original FOV unless explicitly specified (via -crop_unbound or negative -axis extent).

- pad: Analogously to cropping, padding increases the FOV of an image without image interpolation. Pad and crop can be performed simultaneously by specifying signed specifier argument values to the -axis option.

This command encapsulates and extends the functionality of the superseded commands 'mrpad', 'mrcrop' and 'mrresize'. Note the difference in -axis convention used for 'mrcrop' and 'mrpad' (see -axis option description).

Example usages
--------------

-   *Crop and pad the first axis*::

        $ mrgrid in.mif crop -axis 0 10,-5 out.mif

    This removes 10 voxels on the lower and pads with 5 on the upper bound, which is equivalent to padding with the negated specifier (mrgrid in.mif pad -axis 0 -10,5 out.mif).

-   *Right-pad the image to the number of voxels of a reference image*::

        $ mrgrid in.mif pad -as ref.mif -all_axes -axis 3 0,0 out.mif -fill nan

    This pads the image on the upper bound of all axes except for the volume dimension. The headers of in.mif and ref.mif are ignored and the output image uses NAN values to fill in voxels outside the original range of in.mif.

-   *Regrid and interpolate to match the voxel grid of a reference image*::

        $ mrgrid in.mif regrid -template ref.mif -scale 1,1,0.5 out.mif -fill nan

    The -template instructs to regrid in.mif to match the voxel grid of ref.mif (voxel size, grid orientation and voxel centres). The -scale option overwrites the voxel scaling factor yielding voxel sizes in the third dimension that are twice as coarse as those of the template image.

Options
-------

Regridding options (involves image interpolation, applied to spatial axes only)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-template image** match the input image grid (voxel spacing, image size, header transformation) to that of a reference image. The image resolution relative to the template image can be changed with one of -size, -voxel, -scale.

-  **-size dims** define the size (number of voxels) in each spatial dimension for the output image. This should be specified as a comma-separated list.

-  **-voxel size** define the new voxel size for the output image. This can be specified either as a single value to be used for all spatial dimensions, or as a comma-separated list of the size for each voxel dimension.

-  **-scale factor** scale the image resolution by the supplied factor. This can be specified either as a single value to be used for all dimensions, or as a comma-separated list of scale factors for each dimension.

-  **-interp method** set the interpolation method to use when reslicing (choices: nearest, linear, cubic, sinc. Default: cubic).

-  **-oversample factor** set the amount of over-sampling (in the target space) to perform when regridding. This is particularly relevant when downsamping a high-resolution image to a low-resolution image, to avoid aliasing artefacts. This can consist of a single integer, or a comma-separated list of 3 integers if different oversampling factors are desired along the different axes. Default is determined from ratio of voxel dimensions (disabled for nearest-neighbour interpolation).

Pad and crop options (no image interpolation is performed, header transformation is adjusted)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-as reference image** pad or crop the input image on the upper bound to match the specified reference image grid. This operation ignores differences in image transformation between input and reference image.

-  **-uniform number** pad or crop the input image by a uniform number of voxels on all sides

-  **-mask image** crop the input image according to the spatial extent of a mask image. The mask must share a common voxel grid with the input image but differences in image transformations are ignored. Note that even though only 3 dimensions are cropped when using a mask, the bounds are computed by checking the extent for all dimensions. Note that by default a gap of 1 voxel is left at all edges of the image to allow valid trilinear interpolation. This gap can be modified with the -uniform option but by default it does not extend beyond the FOV unless -crop_unbound is used.

-  **-crop_unbound** Allow padding beyond the original FOV when cropping.

-  **-axis index spec** *(multiple uses permitted)* pad or crop the input image along the provided axis (defined by index). The specifier argument defines the number of voxels added or removed on the lower or upper end of the axis (-axis index delta_lower,delta_upper) or acts as a voxel selection range (-axis index start:stop). In both modes, values are relative to the input image (overriding all other extent-specifying options). Negative delta specifier values trigger the inverse operation (pad instead of crop and vice versa) and negative range specifier trigger padding. Note that the deprecated commands 'mrcrop' and 'mrpad' used range-based and delta-based -axis indices, respectively.

-  **-all_axes** Crop or pad all, not just spatial axes.

General options
^^^^^^^^^^^^^^^

-  **-fill number** Use number as the out of bounds value. nan, inf and -inf are valid arguments. (Default: 0.0)

Stride options
^^^^^^^^^^^^^^

-  **-strides spec** specify the strides of the output data in memory; either as a comma-separated list of (signed) integers, or as a template image from which the strides shall be extracted and used. The actual strides produced will depend on whether the output image format can support it.

Data type options
^^^^^^^^^^^^^^^^^

-  **-datatype spec** specify output image data type. Valid choices are: float32, float32le, float32be, float64, float64le, float64be, int64, uint64, int64le, uint64le, int64be, uint64be, int32, uint32, int32le, uint32le, int32be, uint32be, int16, uint16, int16le, uint16le, int16be, uint16be, cfloat32, cfloat32le, cfloat32be, cfloat64, cfloat64le, cfloat64be, int8, uint8, bit.

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



**Author:** Max Pietsch (maximilian.pietsch@kcl.ac.uk) & David Raffelt (david.raffelt@florey.edu.au) & Robert E. Smith (robert.smith@florey.edu.au)

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


