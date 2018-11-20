.. _dwi2fod:

dwi2fod
===================

Synopsis
--------

Estimate fibre orientation distributions from diffusion data using spherical deconvolution

Usage
--------

::

    dwi2fod [ options ]  algorithm dwi response odf [ response odf ... ]

-  *algorithm*: the algorithm to use for FOD estimation. (options are: csd,msmt_csd)
-  *dwi*: the input diffusion-weighted image
-  *response odf*: pairs of input tissue response and output ODF images

Description
-----------

The spherical harmonic coefficients are stored as follows. First, since the signal attenuation profile is real, it has conjugate symmetry, i.e. Y(l,-m) = Y(l,m)* (where * denotes the complex conjugate). Second, the diffusion profile should be antipodally symmetric (i.e. S(x) = S(-x)), implying that all odd l components should be zero. Therefore, only the even elements are computed. Note that the spherical harmonics equations used here differ slightly from those conventionally used, in that the (-1)^m factor has been omitted. This should be taken into account in all subsequent calculations. Each volume in the output image corresponds to a different spherical harmonic component. Each volume will correspond to the following: volume 0: l = 0, m = 0 ; volume 1: l = 2, m = -2 (imaginary part of m=2 SH) ; volume 2: l = 2, m = -1 (imaginary part of m=1 SH) ; volume 3: l = 2, m = 0 ; volume 4: l = 2, m = 1 (real part of m=1 SH) ; volume 5: l = 2, m = 2 (real part of m=2 SH) ; etc...

Options
-------

DW gradient table import options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad file** Provide the diffusion-weighted gradient scheme used in the acquisition in a text file. This should be supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

-  **-fslgrad bvecs bvals** Provide the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format files. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

-  **-bvalue_scaling mode** specifies whether the b-values should be scaled by the square of the corresponding DW gradient norm, as often required for multi-shell or DSI DW acquisition schemes. The default action can also be set in the MRtrix config file, under the BValueScaling entry. Valid choices are yes/no, true/false, 0/1 (default: true).

DW shell selection options
^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-shells b-values** specify one or more b-values to use during processing, as a comma-separated list of the desired approximate b-values (b-values are clustered to allow for small deviations). Note that some commands are incompatible with multiple b-values, and will report an error if more than one b-value is provided. WARNING: note that, even though the b=0 volumes are never referred to as shells in the literature, they still have to be explicitly included in the list of b-values as provided to the -shell option! Several algorithms which include the b=0 volumes in their computations may otherwise return an undesired result.

Options common to more than one algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-directions file** specify the directions over which to apply the non-negativity constraint (by default, the built-in 300 direction set is used). These should be supplied as a text file containing [ az el ] pairs for the directions.

-  **-lmax order** the maximum spherical harmonic order for the output FOD(s).For algorithms with multiple outputs, this should be provided as a comma-separated list of integers, one for each output image; for single-output algorithms, only a single integer should be provided. If omitted, the command will use the highest possible lmax given the diffusion gradient table, up to a maximum of 8.

-  **-mask image** only perform computation within the specified binary brain mask image.

Options for the Constrained Spherical Deconvolution algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-filter spec** the linear frequency filtering parameters used for the initial linear spherical deconvolution step (default = [ 1 1 1 0 0 ]). These should be  supplied as a text file containing the filtering coefficients for each even harmonic order.

-  **-neg_lambda value** the regularisation parameter lambda that controls the strength of the non-negativity constraint (default = 1).

-  **-norm_lambda value** the regularisation parameter lambda that controls the strength of the constraint on the norm of the solution (default = 1).

-  **-threshold value** the threshold below which the amplitude of the FOD is assumed to be zero, expressed as an absolute amplitude (default = 0).

-  **-niter number** the maximum number of iterations to perform for each voxel (default = 50). Use '-niter 0' for a linear unconstrained spherical deconvolution.

Options for the Multi-Shell, Multi-Tissue Constrained Spherical Deconvolution algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-norm_lambda value** the regularisation parameter lambda that controls the strength of the constraint on the norm of the solution (default = 1e-10).

-  **-neg_lambda value** the regularisation parameter lambda that controls the strength of the non-negativity constraint (default = 1e-10).

-  **-predicted_signal image** the predicted dwi image.

Stride options
^^^^^^^^^^^^^^

-  **-strides spec** specify the strides of the output data in memory; either as a comma-separated list of (signed) integers, or as a template image from which the strides shall be extracted and used. The actual strides produced will depend on whether the output image format can support it.

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

* If using csd algorithm:Tournier, J.-D.; Calamante, F. & Connelly, A. Robust determination of the fibre orientation distribution in diffusion MRI: Non-negativity constrained super-resolved spherical deconvolution. NeuroImage, 2007, 35, 1459-1472

* If using msmt_csd algorithm:Jeurissen, B; Tournier, J-D; Dhollander, T; Connelly, A & Sijbers, J. Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data NeuroImage, 2014, 103, 411-426

Tournier, J.-D.; Calamante, F., Gadian, D.G. & Connelly, A. Direct estimation of the fiber orientation density function from diffusion-weighted MRI data using spherical deconvolution.NeuroImage, 2004, 23, 1176-1185

--------------



**Author:** J-Donald Tournier (jdtournier@gmail.com) and Ben Jeurissen (ben.jeurissen@uantwerpen.be)

**Copyright:** Copyright (c) 2008-2018 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/

MRtrix3 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/


