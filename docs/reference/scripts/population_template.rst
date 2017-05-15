.. _population_template:

population_template
===================

Synopsis
--------

Generates an unbiased group-average template from a series of images

Usage
--------

::

    population_template input_dir template [ options ]

-  *input_dir*: The input directory containing all images used to build the template
-  *template*: The output template image

Description
-----------

First a template is optimised with linear registration (rigid or affine, affine is default), then non-linear registration is used to optimise the template further.

Options
-------

Input, output and general options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-type** Specifiy the types of registration stages to perform. Options are "rigid" (perform rigid registration only which might be useful for intra-subject registration in longitudinal analysis), "affine" (perform affine registration) and "nonlinear" as well as cominations of registration types: "rigid_affine", "rigid_nonlinear", "affine_nonlinear", "rigid_affine_nonlinear". Default: rigid_affine_nonlinear

- **-initial_alignment** Method of alignment to form the initial template. Options are "mass" (default), "geometric" and "none".

- **-mask_dir** Optionally input a set of masks inside a single directory, one per input image (with the same file name prefix). Using masks will speed up registration significantly

- **-warp_dir** Output a directory containing warps from each input to the template. If the folder does not exist it will be created

- **-transformed_dir** Output a directory containing the input images transformed to the template. If the folder does not exist it will be created

- **-linear_transformations_dir** Output a directory containing the linear transformations used to generate the template. If the folder does not exist it will be created

- **-template_mask** Output a template mask. Only works in -mask_dir has been input. The template mask is computed as the intersection of all subject masks in template space.

- **-noreorientation** Turn off FOD reorientation in mrregister. Reorientation is on by default if the number of volumes in the 4th dimension corresponds to the number of coefficients in an antipodally symmetric spherical harmonic series (i.e. 6, 15, 28, 45, 66 etc)

Options for the non-linear registration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nl_scale** Specifiy the multi-resolution pyramid used to build the non-linear template, in the form of a list of scale factors (default: 0.3,0.4,0.5,0.6,0.7,0.8,0.9,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0). This implicitly defines the number of template levels

- **-nl_lmax** Specifiy the lmax used for non-linear registration for each scale factor, in the form of a list of integers (default: 2,2,2,2,2,2,2,2,4,4,4,4,4,4,4,4). The list must be the same length as the nl_scale factor list

- **-nl_niter** Specifiy the number of registration iterations used within each level before updating the template, in the form of a list of integers (default: 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5). The list must be the same length as the nl_scale factor list

- **-nl_update_smooth** Regularise the gradient update field with Gaussian smoothing (standard deviation in voxel units, Default 2.0 x voxel_size)

- **-nl_disp_smooth** Regularise the displacement field with Gaussian smoothing (standard deviation in voxel units, Default 1.0 x voxel_size)

- **-nl_grad_step** The gradient step size for non-linear registration (Default: 0.5)

Options for the linear registration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-linear_no_pause** Do not pause the script if a linear registration seems implausible

- **-linear_estimator** Choose estimator for intensity difference metric. Valid choices are: l1 (least absolute: \|x\|), l2 (ordinary least squares), lp (least powers: \|x\|^1.2), Default: l2

- **-rigid_scale** Specifiy the multi-resolution pyramid used to build the rigid template, in the form of a list of scale factors (default: 0.3,0.4,0.6,0.8,1.0,1.0). This and affine_scale implicitly  define the number of template levels

- **-rigid_lmax** Specifiy the lmax used for rigid registration for each scale factor, in the form of a list of integers (default: 2,2,2,4,4,4). The list must be the same length as the linear_scale factor list

- **-rigid_niter** Specifiy the number of registration iterations used within each level before updating the template, in the form of a list of integers (default:50 for each scale). This must be a single number or a list of same length as the linear_scale factor list

- **-affine_scale** Specifiy the multi-resolution pyramid used to build the affine template, in the form of a list of scale factors (default: 0.3,0.4,0.6,0.8,1.0,1.0). This and rigid_scale implicitly define the number of template levels

- **-affine_lmax** Specifiy the lmax used for affine registration for each scale factor, in the form of a list of integers (default: 2,2,2,4,4,4). The list must be the same length as the linear_scale factor list

- **-affine_niter** Specifiy the number of registration iterations used within each level before updating the template, in the form of a list of integers (default:500 for each scale). This must be a single number or a list of same length as the linear_scale factor list

Standard options
^^^^^^^^^^^^^^^^

- **-continue <TempDir> <LastFile>** Continue the script from a previous execution; must provide the temporary directory path, and the name of the last successfully-generated file

- **-force** Force overwrite of output files if pre-existing

- **-help** Display help information for the script

- **-nocleanup** Do not delete temporary files during script, or temporary directory at script completion

- **-nthreads number** Use this number of threads in MRtrix multi-threaded applications (0 disables multi-threading)

- **-tempdir /path/to/tmp/** Manually specify the path in which to generate the temporary directory

- **-quiet** Suppress all console output during script execution

- **-info** Display additional information and progress for every command invoked

- **-debug** Display additional debugging information over and above the output of -info

--------------



**Author:** David Raffelt (david.raffelt@florey.edu.au) & Max Pietsch (maximilian.pietsch@kcl.ac.uk) & Thijs Dhollander (thijs.dhollander@gmail.com)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.

