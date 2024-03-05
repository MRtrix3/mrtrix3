.. _mrregister:

mrregister
===================

Synopsis
--------

Register two images together using a symmetric rigid, affine or non-linear transformation model

Usage
--------

::

    mrregister [ options ]  image1 image2[ contrast1 contrast2 ... ]

-  *image1 image2*: input image 1 ('moving') and input image 2 ('template')
-  *contrast1 contrast2*: optional list of additional input images used as additional contrasts. Can be used multiple times. contrastX and imageX must share the same coordinate system.

Description
-----------

By default this application will perform an affine, followed by non-linear registration.

FOD registration (with apodised point spread reorientation) will be performed by default if the number of volumes in the 4th dimension equals the number of coefficients in an antipodally symmetric spherical harmonic series (e.g. 6, 15, 28 etc). The -no_reorientation option can be used to force reorientation off if required.

Non-linear registration computes warps to map from both image1->image2 and image2->image1. Similar to Avants (2008) Med Image Anal. 12(1): 26-41, registration is performed by matching both the image1 and image2 in a 'midway space'. Warps can be saved as two deformation fields that map directly between image1->image2 and image2->image1, or if using -nl_warp_full as a single 5D file that stores all 4 warps image1->mid->image2, and image2->mid->image1. The 5D warp format stores x,y,z deformations in the 4th dimension, and uses the 5th dimension to index the 4 warps. The affine transforms estimated (to midway space) are also stored as comments in the image header. The 5D warp file can be used to reinitialise subsequent registrations, in addition to transforming images to midway space (e.g. for intra-subject alignment in a 2-time-point longitudinal analysis).

Options
-------

-  **-type choice** the registration type. Valid choices are: rigid, affine, nonlinear, rigid_affine, rigid_nonlinear, affine_nonlinear, rigid_affine_nonlinear (Default: affine_nonlinear)

-  **-transformed image** *(multiple uses permitted)* image1 after registration transformed and regridded to the space of image2. Note that -transformed needs to be repeated for each contrast if multi-contrast registration is used.

-  **-transformed_midway image1_transformed image2_transformed** *(multiple uses permitted)* image1 and image2 after registration transformed and regridded to the midway space. Note that -transformed_midway needs to be repeated for each contrast if multi-contrast registration is used.

-  **-mask1 filename** a mask to define the region of image1 to use for optimisation.

-  **-mask2 filename** a mask to define the region of image2 to use for optimisation.

-  **-nan** use NaN as out of bounds value. (Default: 0.0)

Rigid registration options
^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-rigid file** the output text file containing the rigid transformation as a 4x4 matrix

-  **-rigid_1tomidway file** the output text file containing the rigid transformation that aligns image1 to image2 in their common midway space as a 4x4 matrix

-  **-rigid_2tomidway file** the output text file containing the rigid transformation that aligns image2 to image1 in their common midway space as a 4x4 matrix

-  **-rigid_init_translation type** initialise the translation and centre of rotation; Valid choices are: mass (aligns the centers of mass of both images, default); geometric (aligns geometric image centres); none.

-  **-rigid_init_rotation type** Method to use to initialise the rotation. Valid choices are: search (search for the best rotation using mean squared residuals); moments (rotation based on directions of intensity variance with respect to centre of mass); none (default).

-  **-rigid_init_matrix file** initialise either the rigid, affine, or syn registration with the supplied rigid transformation (as a 4x4 matrix in scanner coordinates).Note that this overrides rigid_init_translation and rigid_init_rotation initialisation

-  **-rigid_scale factor** use a multi-resolution scheme by defining a scale factor for each level using comma-separated values (Default: 0.25,0.5,1.0)

-  **-rigid_niter num** the maximum number of gradient descent iterations per stage. This can be specified either as a single number for all multi-resolution levels, or a single value for each level. (Default: 1000)

-  **-rigid_metric type** valid choices are: diff (intensity differences); Default: diff

-  **-rigid_metric.diff.estimator type** Robust estimator to use during rigid-body registration. Valid choices are: l1 (least absolute: \|x\|); l2 (ordinary least squares); lp (least powers: \|x\|^1.2); none. Default: none.

-  **-rigid_lmax num** explicitly set the lmax to be used per scale factor in rigid FOD registration. By default, FOD registration will use lmax 0,2,4 with default scale factors 0.25,0.5,1.0 respectively. Note that no reorientation will be performed with lmax = 0.

-  **-rigid_log file** write gradient descent parameter evolution to log file

Affine registration options
^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-affine file** the output text file containing the affine transformation as a 4x4 matrix

-  **-affine_1tomidway file** the output text file containing the affine transformation that aligns image1 to image2 in their common midway space as a 4x4 matrix

-  **-affine_2tomidway file** the output text file containing the affine transformation that aligns image2 to image1 in their common midway space as a 4x4 matrix

-  **-affine_init_translation type** initialise the translation and centre of rotation. Valid choices are:  mass (aligns the centers of mass of both images); geometric (aligns geometric image centres); none. (Default: mass)

-  **-affine_init_rotation type** initialise the rotation. Valid choices are: search (search for the best rotation using mean squared residuals); moments (rotation based on directions of intensity variance with respect to centre of mass); none (Default: none).

-  **-affine_init_matrix file** initialise either the affine or syn registration with the supplied affine transformation (as a 4x4 matrix in scanner coordinates). Note that this overrides affine_init_translation and affine_init_rotation initialisation

-  **-affine_scale factor** use a multi-resolution scheme by defining a scale factor for each level using comma separated values (Default: 0.25,0.5,1.0)

-  **-affine_niter num** the maximum number of gradient descent iterations per stage. This can be specified either as a single number for all multi-resolution levels, or a single value for each level. (Default: 1000)

-  **-affine_metric type** valid choices are: diff (intensity differences); Default: diff

-  **-affine_metric.diff.estimator type** Robust estimator to use durring affine registration. Valid choices are: l1 (least absolute: \|x\|); l2 (ordinary least squares); lp (least powers: \|x\|^1.2); none. Default: none.

-  **-affine_lmax num** explicitly set the lmax to be used per scale factor in affine FOD registration. By default FOD registration will use lmax 0,2,4 with default scale factors 0.25,0.5,1.0 respectively. Note that no reorientation will be performed with lmax = 0.

-  **-affine_log file** write gradient descent parameter evolution to log file

Advanced linear transformation initialisation options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-init_translation.unmasked1** disregard mask1 for the translation initialisation (affects 'mass')

-  **-init_translation.unmasked2** disregard mask2 for the translation initialisation (affects 'mass')

-  **-init_rotation.unmasked1** disregard mask1 for the rotation initialisation (affects 'search' and 'moments')

-  **-init_rotation.unmasked2** disregard mask2 for the rotation initialisation (affects 'search' and 'moments')

-  **-init_rotation.search.angles angles** rotation angles for the local search in degrees between 0 and 180. (Default: 2,5,10,15,20)

-  **-init_rotation.search.scale scale** relative size of the images used for the rotation search. (Default: 0.15)

-  **-init_rotation.search.directions num** number of rotation axis for local search. (Default: 250)

-  **-init_rotation.search.run_global** perform a global rather than local initial rotation search.

-  **-init_rotation.search.global.iterations num** number of rotations to investigate (Default: 10000)

Advanced linear registration stage options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-linstage.iterations value(s)** number of iterations for each registration stage. Not to be confused with -rigid_niter or -affine_niter. This can be used to generate intermediate diagnostics images (-linstage.diagnostics.prefix) or to change the cost function optimiser during registration (without the need to repeatedly resize the images). (Default: 1 == no repetition)

-  **-linstage.optimiser.first algorithm** Cost function optimisation algorithm to use at first iteration of all stages. Valid choices: bbgd (Barzilai-Borwein gradient descent); gd (simple gradient descent). (Default: bbgd)

-  **-linstage.optimiser.last algorithm** Cost function optimisation algorithm to use at last iteration of all stages (if there are more than one). Valid choices: bbgd (Barzilai-Borwein gradient descent); gd (simple gradient descent). (Default: bbgd)

-  **-linstage.optimiser.default algorithm** Cost function optimisation algorithm to use at any stage iteration other than first or last iteration. Valid choices: bbgd (Barzilai-Borwein gradient descent); gd (simple gradient descent). (Default: bbgd)

-  **-linstage.diagnostics.prefix prefix** generate diagnostics images after every registration stage

Non-linear registration options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-nl_warp warp1 warp2** the non-linear warp output defined as two deformation fields, where warp1 can be used to transform image1->image2 and warp2 to transform image2->image1. The deformation fields also encapsulate any linear transformation estimated prior to non-linear registration.

-  **-nl_warp_full image** output all warps used during registration. This saves four different warps that map each image to a midway space and their inverses in a single 5D image file. The 4th image dimension indexes the x,y,z component of the deformation vector and the 5th dimension indexes the field in this order: image1->midway, midway->image1, image2->midway, midway->image2. Where image1->midway defines the field that maps image1 onto the midway space using the reverse convention. When linear registration is performed first, the estimated linear transform will be included in the comments of the image header, and therefore the entire linear and non-linear transform can be applied (in either direction) using this output warp file with mrtransform

-  **-nl_init image** initialise the non-linear registration with the supplied warp image. The supplied warp must be in the same format as output using the -nl_warp_full option (i.e. have 4 deformation fields with the linear transforms in the image header)

-  **-nl_scale factor** use a multi-resolution scheme by defining a scale factor for each level using comma separated values (Default: 0.25,0.5,1.0)

-  **-nl_niter num** the maximum number of iterations. This can be specified either as a single number for all multi-resolution levels, or a single value for each level. (Default: 50)

-  **-nl_update_smooth stdev** regularise the gradient update field with Gaussian smoothing (standard deviation in voxel units; Default 2.0)

-  **-nl_disp_smooth stdev** regularise the displacement field with Gaussian smoothing (standard deviation in voxel units; Default 1.0)

-  **-nl_grad_step num** the gradient step size for non-linear registration (Default: 0.5)

-  **-nl_lmax num** explicitly set the lmax to be used per scale factor in non-linear FOD registration. By default, FOD registration will use lmax 0,2,4 with default scale factors 0.25,0.5,1.0 respectively. Note that no reorientation will be performed with lmax = 0.

-  **-diagnostics_image path** write intermediate images for diagnostics purposes

FOD registration options
^^^^^^^^^^^^^^^^^^^^^^^^

-  **-directions file** file containing the directions used for FOD reorientation using apodised point spread functions (Default: built-in 60-direction set)

-  **-noreorientation** turn off FOD reorientation. Reorientation is on by default if the number of volumes in the 4th dimension corresponds to the number of coefficients in an antipodally symmetric spherical harmonic series (i.e. 6, 15, 28, 45, 66 etc)

Multi-contrast options
^^^^^^^^^^^^^^^^^^^^^^

-  **-mc_weights weights** relative weight of images used for multi-contrast registration. Default: 1.0 (equal weighting)

Data type options
^^^^^^^^^^^^^^^^^

-  **-datatype spec** specify output image data type. Valid choices are: float16, float16le, float16be, float32, float32le, float32be, float64, float64le, float64be, int64, uint64, int64le, uint64le, int64be, uint64be, int32, uint32, int32le, uint32le, int32be, uint32be, int16, uint16, int16le, uint16le, int16be, uint16be, cfloat16, cfloat16le, cfloat16be, cfloat32, cfloat32le, cfloat32be, cfloat64, cfloat64le, cfloat64be, int8, uint8, bit.

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

* If FOD registration is being performed: |br|
  Raffelt, D.; Tournier, J.-D.; Fripp, J; Crozier, S.; Connelly, A. & Salvado, O. Symmetric diffeomorphic registration of fibre orientation distributions. NeuroImage, 2011, 56(3), 1171-1180

Raffelt, D.; Tournier, J.-D.; Crozier, S.; Connelly, A. & Salvado, O. Reorientation of fiber orientation distributions using apodized point spread functions. Magnetic Resonance in Medicine, 2012, 67, 844-855

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** David Raffelt (david.raffelt@florey.edu.au) and Max Pietsch (maximilian.pietsch@kcl.ac.uk)

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


