mrregister
===========

Synopsis
--------

::

    mrregister [ options ]  image1 image2

-  *image1*: input image 1 ('moving')
-  *image2*: input image 2 ('template')

Description
-----------

Register two images together using a rigid, affine or a non-linear
transformation model.

By default this application will perform an affine, followed by
non-linear registration.

FOD registration (with apodised point spread reorientation) will be
performed by default if the number of volumes in the 4th dimension
equals the number of coefficients in an antipodally symmetric spherical
harmonic series (e.g. 6, 15, 28 etc). The -no_reorientation option can
be used to force reorientation off if required.

Non-linear registration computes warps to map from both image1->image2
and image2->image1. Similar to Avants (2008) Med Image Anal. 12(1):
26–41, both the image1 and image2 are warped towards a 'middle space'.
Warps are saved in a single 5D file, with the 5th dimension defining the
warp type. See here for more details (TODO). By default the affine
transformation will be saved in the warp image header (use mrinfo to
view). To save the affine transform separately as a text file, use the
-affine option.

Options
-------

-  **-type choice** the registration type. Valid choices are: rigid,
   affine, nonlinear, rigid_affine, rigid_nonlinear,
   affine_nonlinear, rigid_affine_nonlinear (Default:
   affine_nonlinear)

-  **-transformed image** image1 after registration transformed to the
   space of image2

-  **-transformed_midway image1_transformed
   image2_transformed** image1 and image2 after registration
   transformed to the midway space

-  **-mask1 filename** a mask to define the region of image1 to use for
   optimisation.

-  **-mask2 filename** a mask to define the region of image2 to use for
   optimisation.

Rigid registration options
^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-rigid file** the output text file containing the rigid
   transformation as a 4x4 matrix

-  **-rigid_centre type** initialise the centre of rotation and
   initial translation. Valid choices are: mass (which uses the image
   center of mass), geometric (geometric image centre), moments (image
   moments) or none.Default: moments.

-  **-rigid_init file** initialise either the rigid, affine, or syn
   registration with the supplied rigid transformation (as a 4x4
   matrix). Note that this overrides rigid_centre initialisation

-  **-rigid_scale factor** use a multi-resolution scheme by defining a
   scale factor for each level using comma separated values (Default:
   0.5,1)

-  **-rigid_niter num** the maximum number of iterations. This can be
   specified either as a single number for all multi-resolution levels,
   or a single value for each level. (Default: 1000)

-  **-rigid_metric type** valid choices are: l2 (ordinary least
   squares), lp (least powers: |x|^1.2), ncc (normalised
   cross-correlation) Default: ordinary least squares

-  **-rigid_global_search** perform global search for most promising
   starting point. default: false

Affine registration options
^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-affine file** the output text file containing the affine
   transformation that aligns input image 1 to input image 2 as a 4x4
   matrix

-  **-affine_2tomidway file** the output text file containing the
   affine transformation that aligns image2 to image1 in their common
   midway space as a 4x4 matrix

-  **-affine_1tomidway file** the output text file containing the
   affine transformation that aligns image1 to image2 in their common
   midway space as a 4x4 matrix

-  **-affine_centre type** initialise the centre of rotation and
   initial translation. Valid choices are: mass (which uses the image
   center of mass), geometric (geometric image centre), moments (image
   moments) or none.Default: moments. Note that if rigid registration is
   performed first then the affine transform will be initialised with
   the rigid output.

-  **-affine_init file** initialise either the affine, or syn
   registration with the supplied affine transformation (as a 4x4
   matrix). Note that this overrides affine_centre initialisation

-  **-affine_scale factor** use a multi-resolution scheme by defining
   a scale factor for each level using comma separated values (Default:
   0.25,0.5,1.0)

-  **-affine_niter num** the maximum number of iterations. This can be
   specified either as a single number for all multi-resolution levels,
   or a single value for each level. (Default: 1000)

-  **-affine_loop_density num** density of gradient descent 1 (batch)
   to 0.0 (max stochastic) (Default: 1.0)

-  **-affine_repetitions num** number of repetitions with identical
   settings for each scale level

-  **-affine_metric type** valid choices are: diff (intensity
   differences), ncc (normalised cross-correlation) Default: diff

-  **-affine_robust_estimator type** Valid choices are: l1 (least
   absolute: |x|), l2 (ordinary least squares), lp (least powers:
   |x|^1.2), Default: l2

-  **-affine_robust_median** use robust median estimator. default:
   false

-  **-affine_global_search** perform global search for most promising
   starting point. default: false

Non-linear registration options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-nl_warp image** the non-linear output defined as four
   displacement fields in midway space. The 4th image dimension defines
   x,y,z component, and the 5th dimension defines the field in this
   order (image1->midway, midway->image1, image2->midway,
   midway->image2).Where image1->midway defines the field that maps
   image1 onto the midway space using the reverse convention (i.e.
   displacements map midway voxel positions to image1 space).When linear
   registration is performed first, the estimated linear transform will
   be included in the comments of the image header, and therefore the
   entire linear and non-linear transform can be applied using this
   output warp file with mrtransform

-  **-nl_init image** initialise the non-linear registration with the
   supplied warp image. The supplied warp must be in the same format as
   output using the -nl_warp option (i.e. have 4 displacement fields
   with the linear transform in the image header)

-  **-nl_scale factor** use a multi-resolution scheme by defining a
   scale factor for each level using comma separated values (Default:
   0.25,0.5,1.0)

-  **-nl_niter num** the maximum number of iterations. This can be
   specified either as a single number for all multi-resolution levels,
   or a single value for each level. (Default: 50)

-  **-nl_update_smooth stdev** regularise the gradient update field
   with Gaussian smoothing (standard deviation in voxel units, Default
   2.0 x voxel_size)

-  **-nl_disp_smooth stdev** regularise the displacement field with
   Gaussian smoothing (standard deviation in voxel units, Default 1.0 x
   voxel_size)

-  **-nl_grad_step num** the gradient step size for non-linear
   registration (Default: 0.5)

FOD registration options
^^^^^^^^^^^^^^^^^^^^^^^^

-  **-directions file** the directions used for FOD reorienation using
   apodised point spread functions (Default: 60 directions)

-  **-lmax num** explicitly set the lmax to be used in FOD
   registration. By default FOD registration will use lmax 4 SH
   coefficients

-  **-noreorientation** turn off FOD reorientation. Reorientation is on
   by default if the number of volumes in the 4th dimension corresponds
   to the number of coefficients in an antipodally symmetric spherical
   harmonic series (i.e. 6, 15, 28, 45, 66 etc

Data type options
^^^^^^^^^^^^^^^^^

-  **-datatype spec** specify output image data type. Valid choices
   are: float32, float32le, float32be, float64, float64le, float64be,
   int64, uint64, int64le, uint64le, int64be, uint64be, int32, uint32,
   int32le, uint32le, int32be, uint32be, int16, uint16, int16le,
   uint16le, int16be, uint16be, cfloat32, cfloat32le, cfloat32be,
   cfloat64, cfloat64le, cfloat64be, int8, uint8, bit.

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same
   file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded
   applications

-  **-failonwarn** terminate program if a warning is produced

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

-  If FOD registration is being performed:Raffelt, D.; Tournier, J.-D.;
   Fripp, J; Crozier, S.; Connelly, A. & Salvado, O. Symmetric
   diffeomorphic registration of fibre orientation distributions.
   NeuroImage, 2011, 56(3), 1171-1180

Raffelt, D.; Tournier, J.-D.; Crozier, S.; Connelly, A. & Salvado, O.
Reorientation of fiber orientation distributions using apodized point
spread functions. Magnetic Resonance in Medicine, 2012, 67, 844-855

--------------


**Author:** David Raffelt (david.raffelt@florey.edu.au) & Max Pietsch
(maximilian.pietsch@kcl.ac.uk)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org
