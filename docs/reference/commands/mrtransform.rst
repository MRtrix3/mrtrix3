.. _mrtransform:

mrtransform
===================

Synopsis
--------

Apply spatial transformations to an image

Usage
--------

::

    mrtransform [ options ]  input output

-  *input*: input image to be transformed.
-  *output*: the output image.

Description
-----------

If a linear transform is applied without a template image the command will modify the image header transform matrix

FOD reorientation (with apodised point spread functions) will be performed by default if the number of volumes in the 4th dimension equals the number of coefficients in an antipodally symmetric spherical harmonic series (e.g. 6, 15, 28 etc). The -no_reorientation option can be used to force reorientation off if required.

If a DW scheme is contained in the header (or specified separately), and the number of directions matches the number of volumes in the images, any transformation applied using the -linear option will be also be applied to the directions.

Options
-------

Affine transformation options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-linear transform** specify a linear transform to apply, in the form of a 3x4 or 4x4 ascii file. Note the standard 'reverse' convention is used, where the transform maps points in the template image to the moving image. Note that the reverse convention is still assumed even if no -template image is supplied

-  **-flip axes** flip the specified axes, provided as a comma-separated list of indices (0:x, 1:y, 2:z).

-  **-inverse** apply the inverse transformation

-  **-half** apply the matrix square root of the transformation. This can be combined with the inverse option.

-  **-replace file** replace the linear transform of the original image by that specified, rather than applying it to the original image. The specified transform can be either a template image, or a 3x4 or 4x4 ascii file.

-  **-identity** set the header transform of the image to the identity matrix

Regridding options
^^^^^^^^^^^^^^^^^^

-  **-template image** reslice the input image to match the specified template image grid.

-  **-midway_space** reslice the input image to the midway space. Requires either the -template or -warp option. If used with -template and -linear option the input image will be resliced onto the grid halfway between the input and template. If used with the -warp option the input will be warped to the midway space defined by the grid of the input warp (i.e. half way between image1 and image2)

-  **-interp method** set the interpolation method to use when reslicing (choices: nearest, linear, cubic, sinc. Default: cubic).

Non-linear transformation options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-warp image** apply a non-linear 4D deformation field to warp the input image. Each voxel in the deformation field must define the scanner space position that will be used to interpolate the input image during warping (i.e. pull-back/reverse warp convention). If the -template image is also supplied the deformation field will be resliced first to the template image grid. If no -template option is supplied then the output image will have the same image grid as the deformation field. This option can be used in combination with the -affine option, in which case the affine will be applied first)

-  **-warp_full image** warp the input image using a 5D warp file output from mrregister. Any linear transforms in the warp image header will also be applied. The -warp_full option must be used in combination with either the -template option or the -midway_space option. If a -template image is supplied then the full warp will be used. By default the image1->image2 transform will be applied, however the -from 2 option can be used to apply the image2->image1 transform. Use the -midway_space option to warp the input image to the midway space. The -from option can also be used to define which warp to use when transforming to midway space

-  **-from image** used to define which space the input image is when using the -warp_mid option. Use -from 1 to warp from image1 or -from 2 to warp from image2

Fibre orientation distribution handling options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-modulate** modulate FODs during reorientation to preserve the apparent fibre density across fibre bundle widths before and after the transformation

-  **-directions file** directions defining the number and orientation of the apodised point spread functions used in FOD reorientation (Default: 300 directions)

-  **-noreorientation** turn off FOD reorientation. Reorientation is on by default if the number of volumes in the 4th dimension corresponds to the number of coefficients in an antipodally symmetric spherical harmonic series (i.e. 6, 15, 28, 45, 66 etc

DW gradient table import options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad file** Provide the diffusion-weighted gradient scheme used in the acquisition in a text file. This should be supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

-  **-fslgrad bvecs bvals** Provide the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format files. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

-  **-bvalue_scaling mode** specifies whether the b-values should be scaled by the square of the corresponding DW gradient norm, as often required for multi-shell or DSI DW acquisition schemes. The default action can also be set in the MRtrix config file, under the BValueScaling entry. Valid choices are yes/no, true/false, 0/1 (default: true).

Data type options
^^^^^^^^^^^^^^^^^

-  **-datatype spec** specify output image data type. Valid choices are: float32, float32le, float32be, float64, float64le, float64be, int64, uint64, int64le, uint64le, int64be, uint64be, int32, uint32, int32le, uint32le, int32be, uint32be, int16, uint16, int16le, uint16le, int16be, uint16be, cfloat32, cfloat32le, cfloat32be, cfloat64, cfloat64le, cfloat64be, int8, uint8, bit.

-  **-nan** Use NaN as the out of bounds value (Default: 0.0)

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

References
^^^^^^^^^^

* If FOD reorientation is being performed:Raffelt, D.; Tournier, J.-D.; Crozier, S.; Connelly, A. & Salvado, O. Reorientation of fiber orientation distributions using apodized point spread functions. Magnetic Resonance in Medicine, 2012, 67, 844-855

* If FOD modulation is being performed:Raffelt, D.; Tournier, J.-D.; Rose, S.; Ridgway, G.R.; Henderson, R.; Crozier, S.; Salvado, O.; Connelly, A.; Apparent Fibre Density: a novel measure for the analysis of diffusion-weighted magnetic resonance images. NeuroImage, 2012, 15;59(4), 3976-94.

--------------



**Author:** J-Donald Tournier (jdtournier@gmail.com) and David Raffelt (david.raffelt@florey.edu.au) and Max Pietsch (maximilian.pietsch@kcl.ac.uk)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.

