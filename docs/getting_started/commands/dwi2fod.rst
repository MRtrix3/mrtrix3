.. _dwi2fod:

dwi2fod
===========

Synopsis
--------

::

    dwi2fod [ options ]  dwi response SH

-  *dwi*: the input diffusion-weighted image.
-  *response*: a text file containing the diffusion-weighted signal response function coefficients for a single fibre population, 
-  *SH*: the output spherical harmonics coefficients image.

Description
-----------

perform non-negativity constrained spherical deconvolution.

Note that this program makes use of implied symmetries in the diffusion profile. First, the fact the signal attenuation profile is real implies that it has conjugate symmetry, i.e. Y(l,-m) = Y(l,m)* (where * denotes the complex conjugate). Second, the diffusion profile should be antipodally symmetric (i.e. S(x) = S(-x)), implying that all odd l components should be zero. Therefore, this program only computes the even elements.

Note that the spherical harmonics equations used here differ slightly from those conventionally used, in that the (-1)^m factor has been omitted. This should be taken into account in all subsequent calculations.

The spherical harmonic coefficients are stored as follows. First, since the signal attenuation profile is real, it has conjugate symmetry, i.e. Y(l,-m) = Y(l,m)* (where * denotes the complex conjugate). Second, the diffusion profile should be antipodally symmetric (i.e. S(x) = S(-x)), implying that all odd l components should be zero. Therefore, only the even elements are computed. Note that the spherical harmonics equations used here differ slightly from those conventionally used, in that the (-1)^m factor has been omitted. This should be taken into account in all subsequent calculations. Each volume in the output image corresponds to a different spherical harmonic component. Each volume will correspond to the following: volume 0: l = 0, m = 0 ; volume 1: l = 2, m = -2 (imaginary part of m=2 SH) ; volume 2: l = 2, m = -1 (imaginary part of m=1 SH) ; volume 3: l = 2, m = 0 ; volume 4: l = 2, m = 1 (real part of m=1 SH) ; volume 5: l = 2, m = 2 (real part of m=2 SH) ; etc...

Options
-------

DW gradient table import options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad encoding** specify the diffusion-weighted gradient scheme used in the acquisition. The program will normally attempt to use the encoding stored in the image header. This should be supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2.

-  **-fslgrad bvecs bvals** specify the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format.

-  **-bvalue_scaling mode** specifies whether the b-values should be scaled by the square of the corresponding DW gradient norm, as often required for multi-shell or DSI DW acquisition schemes. The default action can also be set in the MRtrix config file, under the BValueScaling entry. Valid choices are yes/no, true/false, 0/1 (default: true).

DW Shell selection options
^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-shell list** specify one or more diffusion-weighted gradient shells to use during processing, as a comma-separated list of the desired approximate b-values. Note that some commands are incompatible with multiple shells, and will throw an error if more than one b-value is provided.

Spherical deconvolution options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-lmax order** set the maximum harmonic order for the output series. By default, the program will use the highest possible lmax given the number of diffusion-weighted images, up to a maximum of 8.

-  **-mask image** only perform computation within the specified binary brain mask image.

-  **-directions file** specify the directions over which to apply the non-negativity constraint (by default, the built-in 300 direction set is used). These should be supplied as a text file containing the [ az el ] pairs for the directions.

-  **-filter spec** the linear frequency filtering parameters used for the initial linear spherical deconvolution step (default = [ 1 1 1 0 0 ]). These should be  supplied as a text file containing the filtering coefficients for each even harmonic order.

-  **-neg_lambda value** the regularisation parameter lambda that controls the strength of the non-negativity constraint (default = 1).

-  **-norm_lambda value** the regularisation parameter lambda that controls the strength of the constraint on the norm of the solution (default = 1).

-  **-threshold value** the threshold below which the amplitude of the FOD is assumed to be zero, expressed as an absolute amplitude (default = 0).

-  **-niter number** the maximum number of iterations to perform for each voxel (default = 50). Use '-niter 0' for a linear unconstrained spherical deconvolution.

Stride options
^^^^^^^^^^^^^^

-  **-stride spec** specify the strides of the output data in memory, as a comma-separated list. The actual strides produced will depend on whether the output image format can support it.

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

Tournier, J.-D.; Calamante, F. & Connelly, A. Robust determination of the fibre orientation distribution in diffusion MRI: Non-negativity constrained super-resolved spherical deconvolution. NeuroImage, 2007, 35, 1459-1472

Tournier, J.-D.; Calamante, F., Gadian, D.G. & Connelly, A. Direct estimation of the fiber orientation density function from diffusion-weighted MRI data using spherical deconvolution.NeuroImage, 2004, 23, 1176-1185

--------------



**Author:** J-Donald Tournier (jdtournier@gmail.com)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

