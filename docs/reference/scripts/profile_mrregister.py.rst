profile_mrregister.py
===========

Synopsis
--------

    profile_mrregister.py [ options ] input_pattern mask_pattern

- *input_pattern*: Input file path. Use wildcards for multiple files
- *mask_pattern*: Mask file name. Lives in same directory as corresponding input file

Description
-----------

Desciption

Options
-------

- **-rigid** perform rigid registration instead of affine.

- **-linear_scale** Specifiy the multi-resolution pyramid used to build the rigid or affine template, in the form of a list of scale factors (default: 0.3,0.4,0.5,0.6,0.7,1.0,1.0,1.0,1.0,1.0). This implicitly defines the number of template levels

- **-linear_lmax** Specifiy the lmax used for rigid or affine registration for each scale factor, in the form of a list of integers (default: 0,0,2,2,2,2,2,2,4,4,4,4,4,4,4,4). The list must be the same length as the affine_scale factor list

- **-nl_scale** Specifiy the multi-resolution pyramid used to build the non-linear template, in the form of a list of scale factors (default: 0.3,0.4,0.5,0.6,0.7,0.8,0.9,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0). This implicitly defines the number of template levels

- **-nl_lmax** Specifiy the lmax used for non-linear registration for each scale factor, in the form of a list of integers (default: 0,0,2,2,2,2,2,2,2,2,4,4,4,4). The list must be the same length as the nl_scale factor list

- **-nl_niter** Specifiy the number of registration iterations used within each level before updating the template, in the form of a list of integers (default:5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5). The list must be the same length as the nl_scale factor list

- **-nl_update_smooth** Regularise the gradient update field with Gaussian smoothing (standard deviation in voxel units, Default 2.0 x voxel_size)

- **-nl_disp_smooth** Regularise the displacement field with Gaussian smoothing (standard deviation in voxel units, Default 1.0 x voxel_size)

- **-nl_grad_step** The gradient step size for non-linear registration (Default: 0.5)

- **-noreorientation** Turn off FOD reorientation. Reorientation is on by default if the number of volumes in the 4th dimension corresponds to the number of coefficients in an antipodally symmetric spherical harmonic series (i.e. 6, 15, 28, 45, 66 etc

- **-initial_alignment** Method of alignment to form the initial template. Options are "mass" (default), "geometric" and "none".

- **-log** log mrregister output to text file

- **-results** write stdout to text file

- **-linear_transform_dir** save linear transformations

- **-mrregister_args** more arguments to pass to mrregister, comma separated enclosed in square brackets: [-debug,-affine_niter,1]

Standard options
^^^^^^^^^^^^^^^^


- **-continue <TempDir> <LastFile>** Continue the script from a previous execution; must provide the temporary directory path, and the name of the last successfully-generated file

- **-force** Force overwrite of output files if pre-existing

- **-help** Display help information for the script

- **-nocleanup** Do not delete temporary directory at script completion

- **-nthreads number** Use this number of threads in MRtrix multi-threaded applications (0 disables multi-threading)

- **-tempdir /path/to/tmp/** Manually specify the path in which to generate the temporary directory

- **-quiet** Suppress all console output during script execution

- **-verbose** Display additional information for every command invoked

References
^^^^^^^^^^



---

**Author:** Max

**Copyright:** 
Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public 
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org
