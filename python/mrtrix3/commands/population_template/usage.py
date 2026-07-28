# Copyright (c) 2008-2026 the MRtrix3 contributors.
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

import pathlib
from mrtrix3 import app, utils #pylint: disable=no-name-in-module

from . import AGGREGATION_MODES, \
              DEFAULT_AFFINE_LMAX, \
              DEFAULT_AFFINE_SCALES, \
              DEFAULT_NL_DISP_SMOOTH, \
              DEFAULT_NL_GRAD_STEP, \
              DEFAULT_NL_LMAX, \
              DEFAULT_NL_NITER, \
              DEFAULT_NL_SCALES, \
              DEFAULT_NL_UPDATE_SMOOTH, \
              DEFAULT_RIGID_LMAX, \
              DEFAULT_RIGID_SCALES, \
              INITIAL_ALIGNMENT, \
              LEAVE_ONE_OUT, \
              LINEAR_ESTIMATORS, \
              REGISTRATION_MODES


class SequenceDirectoryOut(app.Parser.CustomTypeBase):
  def __call__(self, input_value):
    return [app.Parser.make_userpath_object(app.Parser._UserDirOutPathExtras, item) # pylint: disable=protected-access \
            for item in input_value.split(',')]
  @staticmethod
  def _legacytypestring():
    return 'SEQDIROUT'
  @staticmethod
  def _metavar():
    return 'directory_list'


class DirectoryInOrImageOut(app.Parser.CustomTypeBase):
  def __call__(self, input_value):
    if input_value == '-':
      input_value = utils.name_temporary('mif')
      abspath = pathlib.Path(input_value)
      app._STDOUT_IMAGES.append(abspath)
      return abspath # pylint: disable=protected-access
    return app.Parser.make_userpath_object(app.Parser._UserPathExtras, input_value) # pylint: disable=protected-access
  @staticmethod
  def _legacytypestring():
    return 'DIRIN IMAGEOUT'
  @staticmethod
  def _metavar():
    return 'directory_in'



def usage(cmdline): #pylint: disable=unused-variable
  cmdline.set_author('David Raffelt (david.raffelt@florey.edu.au)'
                     ' and Max Pietsch (maximilian.pietsch@kcl.ac.uk)'
                     ' and Thijs Dhollander (thijs.dhollander@gmail.com)')

  cmdline.set_synopsis('Generates an unbiased group-average template from a series of images')
  cmdline.add_description('First a template is optimised with linear registration'
                          ' (rigid and/or affine, both by default),'
                          ' then non-linear registration is used to optimise the template further.')
  cmdline.add_argument('input_dir',
                       'Input directory containing all images of a given contrast',
                       type=DirectoryInOrImageOut(),
                       allow_multiple=True)
  cmdline.add_argument('template',
                       'Output template image',
                       type=app.Parser.ImageOut())
  cmdline.add_example_usage('Multi-contrast registration',
                            'population_template input_WM_ODFs/ output_WM_template.mif input_GM_ODFs/ output_GM_template.mif',
                            'When performing multi-contrast registration,'
                            ' the input directory and corresponding output template image'
                            ' for a given contrast are to be provided as a pair,'
                            ' with the pairs corresponding to different contrasts provided sequentially.')

  options = cmdline.add_option_group('Input, output and general options')
  registration_modes_string = ', '.join(f'"{x}"' for x in REGISTRATION_MODES if '_' in x)
  options.add_option('type',
                     'Specify the types of registration stages to perform.'
                     ' Options are:'
                     ' "rigid" (perform rigid registration only,'
                     ' which might be useful for intra-subject registration in longitudinal analysis);'
                     ' "affine" (perform affine registration);'
                     ' "nonlinear";'
                     f' as well as combinations of registration types: {registration_modes_string}.',
                     choices=REGISTRATION_MODES,
                     default='rigid_affine_nonlinear')
  options.add_option('voxel_size',
                     'Define the template voxel size in mm.'
                     ' Use either a single value for isotropic voxels or 3 comma-separated values.',
                     type=app.Parser.SequenceFloat())
  options.add_option('initial_alignment',
                     'Method of alignment to form the initial template.'
                     ' Options are:'
                     ' "mass";'
                     ' "robust_mass" (requires masks);'
                     ' "geometric";'
                     ' "none".',
                     choices=INITIAL_ALIGNMENT,
                     default='mass')
  options.add_option('mask_dir',
                     'Optionally input a set of masks inside a single directory,'
                     ' one per input image'
                     ' (with the same file name prefix).'
                     ' Using masks will speed up registration significantly.'
                     ' Note that masks are used for registration,'
                     ' not for aggregation.'
                     ' To exclude areas from aggregation,'
                     ' NaN-mask your input images.',
                     type=app.Parser.DirectoryIn())
  options.add_option('warp_dir',
                     'Output a directory containing warps from each input to the template.'
                     ' If the folder does not exist it will be created',
                     type=app.Parser.DirectoryOut())
  options.add_option('transformed_dir',
                     'Output a directory containing the input images transformed to the template.'
                     ' If the folder does not exist it will be created.'
                     ' For multi-contrast registration,'
                     ' provide a comma-separated list of directories.',
                     type=SequenceDirectoryOut())
  options.add_option('linear_transformations_dir',
                     'Output a directory containing the linear transformations'
                     ' used to generate the template.'
                     ' If the folder does not exist it will be created',
                     type=app.Parser.DirectoryOut())
  options.add_option('template_mask',
                     'Output a template mask.'
                     ' Only works if -mask_dir has been input.'
                     ' The template mask is computed as the intersection'
                     ' of all subject masks in template space.',
                     type=app.Parser.ImageOut())
  options.add_option('noreorientation',
                     'Turn off FOD reorientation in mrregister.'
                     ' Reorientation is on by default if the number of volumes in the 4th dimension'
                     ' corresponds to the number of coefficients'
                     ' in an antipodally symmetric spherical harmonic series'
                     ' (i.e. 6, 15, 28, 45, 66 etc)')
  options.add_option('leave_one_out',
                     'Register each input image to a template that does not contain that image.'
                     ' Selection "auto" leaves one out if the number of subjects'
                     ' is larger than 2 and smaller than 15.',
                     choices=LEAVE_ONE_OUT,
                     default='auto')
  options.add_option('aggregate',
                     'Measure used to aggregate information from transformed images to the template image'
                     ' (default: mean)',
                     choices=AGGREGATION_MODES)
  options.add_option('aggregation_weights',
                     'Comma-separated file containing weights used for weighted image aggregation.'
                     ' Each row must contain the identifiers of the input image and its weight.'
                     ' Note that this weighs intensity values not transformations (shape).',
                     type=app.Parser.FileIn())
  options.add_option('nanmask',
                     'Optionally apply masks to (transformed) input images using NaN values'
                     ' to specify include areas for registration and aggregation.'
                     ' Only works if -mask_dir has been input.')
  options.add_option('copy_input',
                     'Copy input images and masks into local scratch directory.')
  options.add_option('delete_temporary_files',
                     'Delete temporary files from scratch directory during template creation.')

  options = cmdline.add_option_group('Multi-contrast options')
  options.add_option('mc_weight_initial_alignment',
                     'Weight contribution of each contrast to the initial alignment.'
                     ' Comma separated,'
                     ' default: 1.0 for each contrast (ie. equal weighting).',
                     type=app.Parser.SequenceFloat())
  options.add_option('mc_weight_rigid',
                     'Weight contribution of each contrast to the objective of rigid registration.'
                     ' Comma separated,'
                     ' default: 1.0 for each contrast (ie. equal weighting)',
                     type=app.Parser.SequenceFloat())
  options.add_option('mc_weight_affine',
                     'Weight contribution of each contrast to the objective of affine registration.'
                     ' Comma separated,'
                     ' default: 1.0 for each contrast (ie. equal weighting)',
                     type=app.Parser.SequenceFloat())
  options.add_option('mc_weight_nl',
                     'Weight contribution of each contrast to the objective of nonlinear registration.'
                     ' Comma separated,'
                     ' default: 1.0 for each contrast (ie. equal weighting)',
                     type=app.Parser.SequenceFloat())

  linoptions = cmdline.add_option_group('Options for the linear registration')
  linoptions.add_option('linear_no_pause',
                        'Do not pause the script if a linear registration seems implausible')
  linoptions.add_option('linear_no_drift_correction',
                        'Deactivate correction of template appearance (scale and shear) over iterations')
  linoptions.add_option('linear_estimator',
                        'Specify estimator for intensity difference metric.'
                        ' Valid choices are:'
                        ' l1 (least absolute: |x|),'
                        ' l2 (ordinary least squares),'
                        ' lp (least powers: |x|^1.2),'
                        ' none (no robust estimator).'
                        ' (default: none)',
                        choices=LINEAR_ESTIMATORS)
  linoptions.add_option('rigid_scale',
                        'Specify the multi-resolution pyramid used to build the rigid template,'
                        ' in the form of a list of scale factors'
                        f' (default: {",".join([str(x) for x in DEFAULT_RIGID_SCALES])}).'
                        ' This and affine_scale implicitly define the number of template levels',
                        type=app.Parser.SequenceFloat())
  linoptions.add_option('rigid_lmax',
                        'Specify the lmax used for rigid registration for each scale factor,'
                        ' in the form of a list of integers'
                        f' (default: {",".join([str(x) for x in DEFAULT_RIGID_LMAX])}).'
                        ' The list must be the same length as the linear_scale factor list',
                        type=app.Parser.SequenceLmax())
  linoptions.add_option('rigid_niter',
                        'Specify the number of registration iterations used'
                        ' within each level before updating the template,'
                        ' in the form of a list of integers'
                        ' (default: 50 for each scale).'
                        ' This must be a single number'
                        ' or a list of same length as the linear_scale factor list',
                        type=app.Parser.SequenceInt())
  linoptions.add_option('affine_scale',
                        'Specify the multi-resolution pyramid used to build the affine template,'
                        ' in the form of a list of scale factors'
                        f' (default: {",".join([str(x) for x in DEFAULT_AFFINE_SCALES])}).'
                        ' This and rigid_scale implicitly define the number of template levels',
                        type=app.Parser.SequenceFloat())
  linoptions.add_option('affine_lmax',
                        'Specify the lmax used for affine registration for each scale factor,'
                        ' in the form of a list of integers'
                        f' (default: {",".join([str(x) for x in DEFAULT_AFFINE_LMAX])}).'
                        ' The list must be the same length as the linear_scale factor list',
                        type=app.Parser.SequenceLmax())
  linoptions.add_option('affine_niter',
                        'Specify the number of registration iterations'
                        ' used within each level before updating the template,'
                        ' in the form of a list of integers'
                        ' (default: 500 for each scale).'
                        ' This must be a single number'
                        ' or a list of same length as the linear_scale factor list',
                        type=app.Parser.SequenceInt())

  nloptions = cmdline.add_option_group('Options for the non-linear registration')
  nloptions.add_option('nl_scale',
                       'Specify the multi-resolution pyramid used to build the non-linear template,'
                       ' in the form of a list of scale factors'
                       f' (default: {",".join([str(x) for x in DEFAULT_NL_SCALES])}).'
                       ' This implicitly defines the number of template levels',
                       type=app.Parser.SequenceFloat())
  nloptions.add_option('nl_lmax',
                       'Specify the lmax used for non-linear registration for each scale factor,'
                       ' in the form of a list of integers'
                       f' (default: {",".join([str(x) for x in DEFAULT_NL_LMAX])}).'
                       ' The list must be the same length as the nl_scale factor list',
                       type=app.Parser.SequenceLmax())
  nloptions.add_option('nl_niter',
                       'Specify the number of registration iterations'
                       ' used within each level before updating the template,'
                       ' in the form of a list of integers'
                       f' (default: {",".join([str(x) for x in DEFAULT_NL_NITER])}).'
                       ' The list must be the same length as the nl_scale factor list',
                       type=app.Parser.SequenceInt())
  nloptions.add_option('nl_update_smooth',
                       'Regularise the gradient update field with Gaussian smoothing'
                       ' (standard deviation in voxel units)',
                       type=app.Parser.Float(0.0),
                       default=DEFAULT_NL_UPDATE_SMOOTH)
  nloptions.add_option('nl_disp_smooth',
                       'Regularise the displacement field with Gaussian smoothing'
                       ' (standard deviation in voxel units)',
                       type=app.Parser.Float(0.0),
                       default=DEFAULT_NL_DISP_SMOOTH)
  nloptions.add_option('nl_grad_step',
                       'The gradient step size for non-linear registration',
                       type=app.Parser.Float(0.0),
                       default=DEFAULT_NL_GRAD_STEP)


# ENH: add option to initialise warps / transformations
