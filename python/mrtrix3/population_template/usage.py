# Copyright (c) 2008-2023 the MRtrix3 contributors.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Covered Software is provided under this License on an "as is"
# basis, without warranty of any kind, either expressed, implied, or
# statutory, including, without limitation, warranties that the
# Covered Software is free of defects, merchantable, fit for a
# particular purpose or non-infringing.
# See the Mozilla Public License v. 2.0 for more details.
#
# For more details, see http://www.mrtrix.org/.

from . import AGGREGATION_MODES, REGISTRATION_MODES
from . import DEFAULT_RIGID_LMAX, DEFAULT_RIGID_SCALES
from . import DEFAULT_AFFINE_LMAX, DEFAULT_AFFINE_SCALES
from . import DEFAULT_NL_LMAX, DEFAULT_NL_NITER, DEFAULT_NL_SCALES

def usage(cmdline): #pylint: disable=unused-variable
  cmdline.set_author('David Raffelt (david.raffelt@florey.edu.au) & Max Pietsch (maximilian.pietsch@kcl.ac.uk) & Thijs Dhollander (thijs.dhollander@gmail.com)')

  cmdline.set_synopsis('Generates an unbiased group-average template from a series of images')
  cmdline.add_description('First a template is optimised with linear registration (rigid and/or affine, both by default), then non-linear registration is used to optimise the template further.')
  cmdline.add_argument('input_dir', nargs='+', help='Directory containing all input images of a given contrast')
  cmdline.add_argument('template', help='Output template image')

  cmdline.add_example_usage('Multi-contrast registration',
                            'population_template input_WM_ODFs/ output_WM_template.mif input_GM_ODFs/ output_GM_template.mif',
                            'When performing multi-contrast registration, the input directory and corresponding output template '
                            'image for a given contrast are to be provided as a pair, '
                            'with the pairs corresponding to different contrasts provided sequentially.')

  options = cmdline.add_argument_group('Multi-contrast options')
  options.add_argument('-mc_weight_initial_alignment', help='Weight contribution of each contrast to the initial alignment. Comma separated, default: 1.0')
  options.add_argument('-mc_weight_rigid', help='Weight contribution of each contrast to the objective of rigid registration. Comma separated, default: 1.0')
  options.add_argument('-mc_weight_affine', help='Weight contribution of each contrast to the objective of affine registration. Comma separated, default: 1.0')
  options.add_argument('-mc_weight_nl', help='Weight contribution of each contrast to the objective of nonlinear registration. Comma separated, default: 1.0')

  linoptions = cmdline.add_argument_group('Options for the linear registration')
  linoptions.add_argument('-linear_no_pause', action='store_true', help='Do not pause the script if a linear registration seems implausible')
  linoptions.add_argument('-linear_no_drift_correction', action='store_true', help='Deactivate correction of template appearance (scale and shear) over iterations')
  linoptions.add_argument('-linear_estimator', help='Specify estimator for intensity difference metric. Valid choices are: l1 (least absolute: |x|), l2 (ordinary least squares), lp (least powers: |x|^1.2), Default: None (no robust estimator used)')
  linoptions.add_argument('-rigid_scale', help='Specify the multi-resolution pyramid used to build the rigid template, in the form of a list of scale factors (default: %s). This and affine_scale implicitly  define the number of template levels' % ','.join([str(x) for x in DEFAULT_RIGID_SCALES]))
  linoptions.add_argument('-rigid_lmax', help='Specify the lmax used for rigid registration for each scale factor, in the form of a list of integers (default: %s). The list must be the same length as the linear_scale factor list' % ','.join([str(x) for x in DEFAULT_RIGID_LMAX]))
  linoptions.add_argument('-rigid_niter', help='Specify the number of registration iterations used within each level before updating the template, in the form of a list of integers (default:50 for each scale). This must be a single number or a list of same length as the linear_scale factor list')
  linoptions.add_argument('-affine_scale', help='Specify the multi-resolution pyramid used to build the affine template, in the form of a list of scale factors (default: %s). This and rigid_scale implicitly define the number of template levels' % ','.join([str(x) for x in DEFAULT_AFFINE_SCALES]))
  linoptions.add_argument('-affine_lmax', help='Specify the lmax used for affine registration for each scale factor, in the form of a list of integers (default: %s). The list must be the same length as the linear_scale factor list' % ','.join([str(x) for x in DEFAULT_AFFINE_LMAX]))
  linoptions.add_argument('-affine_niter', help='Specify the number of registration iterations used within each level before updating the template, in the form of a list of integers (default:500 for each scale). This must be a single number or a list of same length as the linear_scale factor list')

  nloptions = cmdline.add_argument_group('Options for the non-linear registration')
  nloptions.add_argument('-nl_scale', help='Specify the multi-resolution pyramid used to build the non-linear template, in the form of a list of scale factors (default: %s). This implicitly defines the number of template levels' % ','.join([str(x) for x in DEFAULT_NL_SCALES]))
  nloptions.add_argument('-nl_lmax', help='Specify the lmax used for non-linear registration for each scale factor, in the form of a list of integers (default: %s). The list must be the same length as the nl_scale factor list' % ','.join([str(x) for x in DEFAULT_NL_LMAX]))
  nloptions.add_argument('-nl_niter', help='Specify the number of registration iterations used within each level before updating the template, in the form of a list of integers (default: %s). The list must be the same length as the nl_scale factor list' % ','.join([str(x) for x in DEFAULT_NL_NITER]))
  nloptions.add_argument('-nl_update_smooth', default='2.0', help='Regularise the gradient update field with Gaussian smoothing (standard deviation in voxel units, Default 2.0 x voxel_size)')
  nloptions.add_argument('-nl_disp_smooth', default='1.0', help='Regularise the displacement field with Gaussian smoothing (standard deviation in voxel units, Default 1.0 x voxel_size)')
  nloptions.add_argument('-nl_grad_step', default='0.5', help='The gradient step size for non-linear registration (Default: 0.5)')

  options = cmdline.add_argument_group('Input, output and general options')
  options.add_argument('-type', help='Specify the types of registration stages to perform. Options are "rigid" (perform rigid registration only which might be useful for intra-subject registration in longitudinal analysis), "affine" (perform affine registration) and "nonlinear" as well as cominations of registration types: %s. Default: rigid_affine_nonlinear' % ', '.join('"' + x + '"' for x in REGISTRATION_MODES if "_" in x), default='rigid_affine_nonlinear')
  options.add_argument('-voxel_size', help='Define the template voxel size in mm. Use either a single value for isotropic voxels or 3 comma separated values.')
  options.add_argument('-initial_alignment', default='mass', help='Method of alignment to form the initial template. Options are "mass" (default), "robust_mass" (requires masks), "geometric" and "none".')
  options.add_argument('-mask_dir', help='Optionally input a set of masks inside a single directory, one per input image (with the same file name prefix). Using masks will speed up registration significantly. Note that masks are used for registration, not for aggregation. To exclude areas from aggregation, NaN-mask your input images.')
  options.add_argument('-warp_dir', help='Output a directory containing warps from each input to the template. If the folder does not exist it will be created')
  options.add_argument('-transformed_dir', help='Output a directory containing the input images transformed to the template. If the folder does not exist it will be created. For multi-contrast registration, provide comma separated list of directories.')
  options.add_argument('-linear_transformations_dir', help='Output a directory containing the linear transformations used to generate the template. If the folder does not exist it will be created')
  options.add_argument('-template_mask', help='Output a template mask. Only works if -mask_dir has been input. The template mask is computed as the intersection of all subject masks in template space.')
  options.add_argument('-noreorientation', action='store_true', help='Turn off FOD reorientation in mrregister. Reorientation is on by default if the number of volumes in the 4th dimension corresponds to the number of coefficients in an antipodally symmetric spherical harmonic series (i.e. 6, 15, 28, 45, 66 etc)')
  options.add_argument('-leave_one_out', help='Register each input image to a template that does not contain that image. Valid choices: 0, 1, auto. (Default: auto (true if n_subjects larger than 2 and smaller than 15)) ')
  options.add_argument('-aggregate', help='Measure used to aggregate information from transformed images to the template image. Valid choices: %s. Default: mean' % ', '.join(AGGREGATION_MODES))
  options.add_argument('-aggregation_weights', help='Comma separated file containing weights used for weighted image aggregation. Each row must contain the identifiers of the input image and its weight. Note that this weighs intensity values not transformations (shape).')
  options.add_argument('-nanmask', action='store_true', help='Optionally apply masks to (transformed) input images using NaN values to specify include areas for registration and aggregation. Only works if -mask_dir has been input.')
  options.add_argument('-copy_input', action='store_true', help='Copy input images and masks into local scratch directory.')
  options.add_argument('-delete_temporary_files', action='store_true', help='Delete temporary files from scratch directory during template creation.')
