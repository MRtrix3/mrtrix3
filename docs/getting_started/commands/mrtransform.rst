mrtransform
===========

Synopsis
--------

::

    mrtransform [ options ]  input output

-  *input*: input image to be transformed.
-  *output*: the output image.

Description
-----------

apply spatial transformations to an image. 

If a linear transform is applied without a template image the command will modify the image header transform matrix

FOD reorientation (with apodised point spread functions) will be performed by default if the number of volumes in the 4th dimension equals the number of coefficients in an antipodally symmetric spherical harmonic series (e.g. 6, 15, 28 etc). The -no_reorientation option can be used to force reorientation off if required.

If a DW scheme is contained in the header (or specified separately), and the number of directions matches the number of volumes in the images, any transformation applied using the -linear option will be also be applied to the directions.

Options
-------

Affine transformation options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-linear transform** specify a 4x4 linear transform to apply, in the form of a 4x4 ascii file. Note the standard 'reverse' convention is used, where the transform maps points in the template image to the moving image. Note that the reverse convention is still assumed even if no -template image is supplied

-  **-flip axes** flip the specified axes, provided as a comma-separated list of indices (0:x, 1:y, 2:z).

-  **-inverse** apply the inverse transformation

-  **-replace** replace the linear transform of the original image by that specified, rather than applying it to the original image. If no -linear transform is specified then the header transform is replaced with an identity transform.

Regridding options
^^^^^^^^^^^^^^^^^^

-  **-template image** reslice the input image to match the specified template image grid.

-  **-interp method** set the interpolation method to use when reslicing (choices: nearest, linear, cubic, sinc. Default: cubic).

Non-linear transformation options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-warp image** apply a non-linear deformation field to warp the input image. If the -template image is also supplied the warp field will be resliced first to the template image grid. If no -template option is supplied then the output image will have the same image grid as the warp.

Fibre orientation distribution handling options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-modulate** modulate the FOD during reorientation to preserve the apparent fibre density

-  **-directions file** directions defining the number and orientation of the apodised point spread functions used in FOD reorientation(Default: 60 directions)

-  **-noreorientation** turn off FOD reorientation. Reorientation is on by default if the number of volumes in the 4th dimension corresponds to the number of coefficients in an antipodally symmetric spherical harmonic series (i.e. 6, 15, 28, 45, 66 etc

DW gradient table import options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad encoding** specify the diffusion-weighted gradient scheme used in the acquisition. The program will normally attempt to use the encoding stored in the image header. This should be supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2.

-  **-fslgrad bvecs bvals** specify the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format.

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

-  **-nthreads number** use this number of threads in multi-threaded applications

-  **-failonwarn** terminate program if a warning is produced

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

* If FOD reorientation is being performed:Raffelt, D.; Tournier, J.-D.; Crozier, S.; Connelly, A. & Salvado, O. Reorientation of fiber orientation distributions using apodized point spread functions. Magnetic Resonance in Medicine, 2012, 67, 844-855

* If FOD modulation is being performed:Raffelt, D.; Tournier, J.-D.; Rose, S.; Ridgway, G.R.; Henderson, R.; Crozier, S.; Salvado, O.; Connelly, A.; Apparent Fibre Density: a novel measure for the analysis of diffusion-weighted magnetic resonance images. NeuroImage, 2012, 15;59(4), 3976-94.

--------------



**Author:** J-Donald Tournier (jdtournier@gmail.com) & David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

