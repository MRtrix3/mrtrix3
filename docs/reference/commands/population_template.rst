.. _population_template:

population_template
===================

Synopsis
--------

Generates an unbiased group-average template from a series of images

Usage
-----

::

    population_template input_dir template [ options ]

-  *input_dir*: Input directory containing all images of a given contrast
-  *template*: Output template image

Description
-----------

First a template is optimised with linear registration (rigid and/or affine, both by default), then non-linear registration is used to optimise the template further.

Example usages
--------------

-   *Multi-contrast registration*::

        $ population_template input_WM_ODFs/ output_WM_template.mif input_GM_ODFs/ output_GM_template.mif

    When performing multi-contrast registration, the input directory and corresponding output template image for a given contrast are to be provided as a pair, with the pairs corresponding to different contrasts provided sequentially.

Options
-------

Input, output and general options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-type choice** Specify the types of registration stages to perform. Options are: "rigid" (perform rigid registration only, which might be useful for intra-subject registration in longitudinal analysis); "affine" (perform affine registration); "nonlinear"; as well as combinations of registration types: "rigid_affine", "rigid_nonlinear", "affine_nonlinear", "rigid_affine_nonlinear". Default: rigid_affine_nonlinear

- **-voxel_size values** Define the template voxel size in mm. Use either a single value for isotropic voxels or 3 comma-separated values.

- **-initial_alignment choice** Method of alignment to form the initial template. Options are: "mass" (default); "robust_mass" (requires masks); "geometric"; "none".

- **-mask_dir directory** Optionally input a set of masks inside a single directory, one per input image (with the same file name prefix). Using masks will speed up registration significantly. Note that masks are used for registration, not for aggregation. To exclude areas from aggregation, NaN-mask your input images.

- **-warp_dir directory** Output a directory containing warps from each input to the template. If the folder does not exist it will be created

- **-transformed_dir directory_list** Output a directory containing the input images transformed to the template. If the folder does not exist it will be created. For multi-contrast registration, provide a comma-separated list of directories.

- **-linear_transformations_dir directory** Output a directory containing the linear transformations used to generate the template. If the folder does not exist it will be created

- **-template_mask image** Output a template mask. Only works if -mask_dir has been input. The template mask is computed as the intersection of all subject masks in template space.

- **-noreorientation** Turn off FOD reorientation in mrregister. Reorientation is on by default if the number of volumes in the 4th dimension corresponds to the number of coefficients in an antipodally symmetric spherical harmonic series (i.e. 6, 15, 28, 45, 66 etc)

- **-leave_one_out choice** Register each input image to a template that does not contain that image. Valid choices: 0, 1, auto. (Default: auto (true if n_subjects larger than 2 and smaller than 15))

- **-aggregate choice** Measure used to aggregate information from transformed images to the template image. Valid choices: mean, median. Default: mean

- **-aggregation_weights file** Comma-separated file containing weights used for weighted image aggregation. Each row must contain the identifiers of the input image and its weight. Note that this weighs intensity values not transformations (shape).

- **-nanmask** Optionally apply masks to (transformed) input images using NaN values to specify include areas for registration and aggregation. Only works if -mask_dir has been input.

- **-copy_input** Copy input images and masks into local scratch directory.

- **-delete_temporary_files** Delete temporary files from scratch directory during template creation.

Options for the non-linear registration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nl_scale values** Specify the multi-resolution pyramid used to build the non-linear template, in the form of a list of scale factors (default: 0.3,0.4,0.5,0.6,0.7,0.8,0.9,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0). This implicitly defines the number of template levels

- **-nl_lmax values** Specify the lmax used for non-linear registration for each scale factor, in the form of a list of integers (default: 2,2,2,2,2,2,2,2,4,4,4,4,4,4,4,4). The list must be the same length as the nl_scale factor list

- **-nl_niter values** Specify the number of registration iterations used within each level before updating the template, in the form of a list of integers (default: 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5). The list must be the same length as the nl_scale factor list

- **-nl_update_smooth value** Regularise the gradient update field with Gaussian smoothing (standard deviation in voxel units, Default 2.0 x voxel_size)

- **-nl_disp_smooth value** Regularise the displacement field with Gaussian smoothing (standard deviation in voxel units, Default 1.0 x voxel_size)

- **-nl_grad_step value** The gradient step size for non-linear registration (Default: 0.5)

Options for the linear registration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-linear_no_pause** Do not pause the script if a linear registration seems implausible

- **-linear_no_drift_correction** Deactivate correction of template appearance (scale and shear) over iterations

- **-linear_estimator choice** Specify estimator for intensity difference metric. Valid choices are: l1 (least absolute: \|x\|), l2 (ordinary least squares), lp (least powers: \|x\|^1.2), none (no robust estimator). Default: none.

- **-rigid_scale values** Specify the multi-resolution pyramid used to build the rigid template, in the form of a list of scale factors (default: 0.3,0.4,0.6,0.8,1.0,1.0). This and affine_scale implicitly define the number of template levels

- **-rigid_lmax values** Specify the lmax used for rigid registration for each scale factor, in the form of a list of integers (default: 2,2,2,4,4,4). The list must be the same length as the linear_scale factor list

- **-rigid_niter values** Specify the number of registration iterations used within each level before updating the template, in the form of a list of integers (default: 50 for each scale). This must be a single number or a list of same length as the linear_scale factor list

- **-affine_scale values** Specify the multi-resolution pyramid used to build the affine template, in the form of a list of scale factors (default: 0.3,0.4,0.6,0.8,1.0,1.0). This and rigid_scale implicitly define the number of template levels

- **-affine_lmax values** Specify the lmax used for affine registration for each scale factor, in the form of a list of integers (default: 2,2,2,4,4,4). The list must be the same length as the linear_scale factor list

- **-affine_niter values** Specify the number of registration iterations used within each level before updating the template, in the form of a list of integers (default: 500 for each scale). This must be a single number or a list of same length as the linear_scale factor list

Multi-contrast options
^^^^^^^^^^^^^^^^^^^^^^

- **-mc_weight_initial_alignment values** Weight contribution of each contrast to the initial alignment. Comma separated, default: 1.0 for each contrast (ie. equal weighting).

- **-mc_weight_rigid values** Weight contribution of each contrast to the objective of rigid registration. Comma separated, default: 1.0 for each contrast (ie. equal weighting)

- **-mc_weight_affine values** Weight contribution of each contrast to the objective of affine registration. Comma separated, default: 1.0 for each contrast (ie. equal weighting)

- **-mc_weight_nl values** Weight contribution of each contrast to the objective of nonlinear registration. Comma separated, default: 1.0 for each contrast (ie. equal weighting)

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify an existing directory in which to generate the scratch directory.

- **-continue ScratchDir LastFile** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

Standard options
^^^^^^^^^^^^^^^^

- **-info** display information messages.

- **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

- **-debug** display debugging messages.

- **-force** force overwrite of output files.

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

- **-config key value**  *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

- **-help** display this information page and exit.

- **-version** display version information and exit.

References
^^^^^^^^^^

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** David Raffelt (david.raffelt@florey.edu.au) and Max Pietsch (maximilian.pietsch@kcl.ac.uk) and Thijs Dhollander (thijs.dhollander@gmail.com)

**Copyright:** Copyright (c) 2008-2025 the MRtrix3 contributors.

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

