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

import math, os, shutil
from mrtrix3 import CONFIG, MRtrixError  #pylint: disable=no-name-in-module
from mrtrix3 import app, fsl, image, matrix, path, run #pylint: disable=no-name-in-module
from . import LMAXES_MULTI, LMAXES_SINGLE, MASK_ALGO_DEFAULT, MASK_ALGOS

def execute(): #pylint: disable=unused-variable

  # Check user inputs
  if app.ARGS.max_iters < 0:
    raise MRtrixError('Maximum number of iterations must be a non-negative integer')
  lmax = None
  if app.ARGS.lmax:
    try:
      lmax = [int(i) for i in app.ARGS.lmax.split(',')]
    except ValueError as exc:
      raise MRtrixError('Values provided to -lmax option must be a comma-separated list of integers') from exc
    if any(value < 0 or value % 2 for value in lmax):
      raise MRtrixError('lmax values must be non-negative even integers')
    if len(lmax) not in [2, 3]:
      raise MRtrixError('Length of lmax vector expected to be either 2 or 3')
  if app.ARGS.dice <= 0.0 or app.ARGS.dice > 1.0:
    raise MRtrixError('Dice coefficient for convergence detection must lie in the range (0.0, 1.0]')
  if app.ARGS.reference <= 0.0:
    raise MRtrixError('Reference intensity must be positive')

  # Check what masking agorithm is going to be used
  mask_algo = MASK_ALGO_DEFAULT
  if app.ARGS.mask_algo:
    mask_algo = app.ARGS.mask_algo
  elif 'DwibiasnormmaskMaskAlgorithm' in CONFIG:
    mask_algo = CONFIG['DwibiasnormmaskMaskAlgorithm']
    if not mask_algo in MASK_ALGOS:
      raise MRtrixError('Invalid masking algorithm selection "%s" in MRtrix config file' % mask_algo)
    app.console('"%s" algorithm will be used for brain masking during iteration as specified in config file' % mask_algo)
  else:
    app.console('Default "%s" algorithm will be used for brain masking during iteration' % MASK_ALGO_DEFAULT)

  # Check mask algorithm, including availability of external software if necessary
  for mask_algo, software, command in [('fslbet', 'FSL', 'bet'), ('hdbet', 'HD-BET', 'hd-bet'), ('synthstrip', 'FreeSurfer', 'mri_synthstrip')]:
    if app.ARGS.mask_algo == mask_algo and not shutil.which(command):
      raise MRtrixError(software + ' command "' + command + '" not found; cannot use for internal mask calculations')

  app.check_output_path(app.ARGS.output_dwi)
  app.check_output_path(app.ARGS.output_mask)
  app.make_scratch_dir()

  grad_import_option = app.read_dwgrad_import_options()
  run.command('mrconvert ' + path.from_user(app.ARGS.input) + ' '
              + path.to_scratch('input.mif') + grad_import_option)

  if app.ARGS.init_mask:
    run.command('mrconvert ' + path.from_user(app.ARGS.init_mask) + ' '
                + path.to_scratch('dwi_mask_init.mif') + ' -datatype bit')

  app.goto_scratch_dir()


  # Check inputs
  # Make sure it's actually a DWI that's been passed
  dwi_header = image.Header('input.mif')
  if len(dwi_header.size()) != 4:
    raise MRtrixError('Input image must be a 4D image')
  if 'dw_scheme' not in dwi_header.keyval():
    raise MRtrixError('No valid DW gradient scheme provided or present in image header')
  if len(dwi_header.keyval()['dw_scheme']) != dwi_header.size()[3]:
    raise MRtrixError('DW gradient scheme contains different number of entries ('
                      + str(len(dwi_header.keyval()['dw_scheme']))
                      + ' to number of volumes in DWIs (' + dwi_header.size()[3] + ')')

  # Determine whether we are working with single-shell or multi-shell data
  bvalues = [
      int(round(float(value)))
      for value in image.mrinfo('input.mif', 'shell_bvalues') \
                                .strip().split()]
  multishell = (len(bvalues) > 2)
  if lmax is None:
    lmax = LMAXES_MULTI if multishell else LMAXES_SINGLE
  elif len(lmax) == 3 and not multishell:
    raise MRtrixError('User specified 3 lmax values for three-tissue decomposition, but input DWI is not multi-shell')

  # Create a mask of voxels where the input data contain positive values;
  #   we want to make sure that these never end up included in the output mask
  run.command('mrmath input.mif max - -axis 3 | '
              'mrthreshold - -abs 0 -comparison gt input_pos_mask.mif')

  # Generate an initial brain mask if required, or check the initial mask if provided by the user
  if app.ARGS.init_mask:
    if not image.match('input.mif', 'dwi_mask_init.mif', up_to_dim=3):
      raise MRtrixError('Provided mask image does not match input DWI')
  else:
    app.debug('Performing intial DWI brain masking')
    run.command('dwi2mask '
                + CONFIG['Dwi2maskAlgorithm']
                + ' input.mif dwi_mask_init.mif')

  # Combined RF estimation / CSD / mtnormalise / mask revision
  class Tissue(object): #pylint: disable=useless-object-inheritance
    def __init__(self, name, index):
      self.name = name
      iter_string = '_iter' + str(index)
      self.tissue_rf = 'response_' + name + iter_string + '.txt'
      self.fod_init = 'FODinit_' + name + iter_string + '.mif'
      self.fod_norm = 'FODnorm_' + name + iter_string + '.mif'


  app.debug('Commencing iterative DWI bias field correction and brain masking with '
            + ('a maximum of ' + str(app.ARGS.max_iters) if app.ARGS.max_iters else 'no limit on number of ') + ' iterations')

  dwi_image = 'input.mif'
  dwi_mask_image = 'dwi_mask_init.mif'
  bias_field_image = None
  tissue_sum_image = None
  iteration = 0
  step = 'initialisation'
  prev_dice_coefficient = 0.0
  total_scaling_factor = 1.0

  def msg():
    return 'Iteration {0}; {1} step; previous Dice coefficient {2}' \
            .format(iteration, step, prev_dice_coefficient)
  progress = app.ProgressBar(msg)

  iteration = 1
  while True:
    iter_string = '_iter' + str(iteration)
    tissues = [Tissue('WM', iteration),
               Tissue('GM', iteration),
               Tissue('CSF', iteration)]

    step = 'dwi2response'
    progress.increment()
    run.command('dwi2response dhollander '
                + dwi_image
                + ' -mask '
                + dwi_mask_image
                + ' '
                + ' '.join(tissue.tissue_rf for tissue in tissues))


    # Immediately remove GM if we can't deal with it
    if not multishell:
      app.cleanup(tissues[1].tissue_rf)
      tissues = tissues[::2]

    step = 'dwi2fod'
    progress.increment()
    app.debug('Performing CSD with lmax values: ' + ','.join(str(item) for item in lmax))
    run.command('dwi2fod msmt_csd '
                + dwi_image
                + ' -lmax ' + ','.join(str(item) for item in lmax)
                + ' '
                + ' '.join(tissue.tissue_rf + ' ' + tissue.fod_init
                           for tissue in tissues))

    step = 'maskfilter'
    progress.increment()
    eroded_mask = os.path.splitext(dwi_mask_image)[0] + '_eroded.mif'
    run.command('maskfilter ' + dwi_mask_image + ' erode ' + eroded_mask)

    step = 'mtnormalise'
    progress.increment()
    old_bias_field_image = bias_field_image
    bias_field_image = 'field' + iter_string + '.mif'
    factors_path = 'factors' + iter_string + '.txt'

    run.command('mtnormalise -balanced'
                + ' -mask ' + eroded_mask
                + ' -check_norm ' + bias_field_image
                + ' -check_factors ' + factors_path
                + ' '
                + ' '.join(tissue.fod_init + ' ' + tissue.fod_norm
                           for tissue in tissues))
    app.cleanup([tissue.fod_init for tissue in tissues])
    app.cleanup(eroded_mask)

    app.debug('Iteration ' + str(iteration) + ', '
                + 'applying estimated bias field and appropiate scaling factor...')
    csf_rf = matrix.load_matrix(tissues[-1].tissue_rf)
    csf_rf_bzero_lzero = csf_rf[0][0]
    app.cleanup([tissue.tissue_rf for tissue in tissues])
    balance_factors = matrix.load_vector(factors_path)
    csf_balance_factor = balance_factors[-1]
    app.cleanup(factors_path)
    scale_multiplier = (app.ARGS.reference * math.sqrt(4.0*math.pi)) / \
                       (csf_rf_bzero_lzero / csf_balance_factor)
    new_dwi_image = 'dwi' + iter_string + '.mif'
    run.command('mrcalc ' + dwi_image + ' '
                + bias_field_image + ' -div '
                + str(scale_multiplier) + ' -mult '
                + new_dwi_image)

    old_dwi_image = dwi_image
    dwi_image = new_dwi_image

    old_tissue_sum_image = tissue_sum_image
    tissue_sum_image = 'tissue_sum' + iter_string + '.mif'

    app.debug('Iteration ' + str(iteration) + ', '
                + 'revising brain mask...')
    step = 'masking'
    progress.increment()

    run.command('mrconvert '
                + tissues[0].fod_norm
                + ' -coord 3 0 - |'
                + ' mrmath - '
                + ' '.join(tissue.fod_norm for tissue in tissues[1:])
                + ' sum - | '
                + 'mrcalc - ' + str(math.sqrt(4.0 * math.pi)) + ' -mult '
                + tissue_sum_image)
    app.cleanup([tissue.fod_norm for tissue in tissues])

    new_dwi_mask_image = 'dwi_mask' + iter_string + '.mif'
    tissue_sum_image_nii = None
    new_dwi_mask_image_nii = None
    if mask_algo in ['fslbet', 'hdbet', 'synthstrip']:
      tissue_sum_image_nii = os.path.splitext(tissue_sum_image)[0] + '.nii'
      run.command('mrconvert ' + tissue_sum_image + ' ' + tissue_sum_image_nii)
      new_dwi_mask_image_nii = os.path.splitext(new_dwi_mask_image)[0] + '.nii'
    if mask_algo == 'dwi2mask':
      run.command('dwi2mask ' + CONFIG.get('Dwi2maskAlgorithm', 'legacy') + ' ' + new_dwi_image + ' ' + new_dwi_mask_image)
    elif mask_algo == 'fslbet':
      run.command('bet ' + tissue_sum_image_nii + ' ' + new_dwi_mask_image_nii + ' -R -m')
      app.cleanup(fsl.find_image(os.path.splitext(new_dwi_mask_image_nii)[0]))
      new_dwi_mask_image_nii = fsl.find_image(os.path.splitext(new_dwi_mask_image_nii)[0] + '_mask')
      run.command('mrcalc ' + new_dwi_mask_image_nii + ' input_pos_mask.mif -mult ' + new_dwi_mask_image)
    elif mask_algo == 'hdbet':
      try:
        run.command('hd-bet -i ' + tissue_sum_image_nii)
      except run.MRtrixCmdError as e_gpu:
        try:
          run.command('hd-bet -i ' + tissue_sum_image_nii + ' -device cpu -mode fast -tta 0')
        except run.MRtrixCmdError as e_cpu:
          raise run.MRtrixCmdError('hd-bet', 1, e_gpu.stdout + e_cpu.stdout, e_gpu.stderr + e_cpu.stderr)
      new_dwi_mask_image_nii = os.path.splitext(tissue_sum_image)[0] + '_bet_mask.nii.gz'
      run.command('mrcalc ' + new_dwi_mask_image_nii + ' input_pos_mask.mif -mult ' + new_dwi_mask_image)
    elif mask_algo in ['mrthreshold', 'threshold']:
      mrthreshold_abs_option = ' -abs 0.5' if mask_algo == 'threshold' else ''
      run.command('mrthreshold '
                  + tissue_sum_image
                  + mrthreshold_abs_option
                  + ' - |'
                  + ' maskfilter - connect -largest - |'
                  + ' mrcalc 1 - -sub - -datatype bit |'
                  + ' maskfilter - connect -largest - |'
                  + ' mrcalc 1 - -sub - -datatype bit |'
                  + ' maskfilter - clean - |'
                  + ' mrcalc - input_pos_mask.mif -mult '
                  + new_dwi_mask_image
                  + ' -datatype bit')
    elif mask_algo == 'synthstrip':
      run.command('mri_synthstrip -i ' + tissue_sum_image_nii + ' --mask ' + new_dwi_mask_image_nii)
      run.command('mrcalc ' + new_dwi_mask_image_nii + ' input_pos_mask.mif -mult ' + new_dwi_mask_image)
    else:
      assert False
    if tissue_sum_image_nii:
      app.cleanup(tissue_sum_image_nii)
    if new_dwi_mask_image_nii:
      app.cleanup(new_dwi_mask_image_nii)

    step = 'mask comparison'
    progress.increment()
    dwi_old_mask_count = image.statistics(dwi_mask_image,
                                          mask=dwi_mask_image).count
    dwi_new_mask_count = image.statistics(new_dwi_mask_image,
                                          mask=new_dwi_mask_image).count
    app.debug('Old mask voxel count: ' + str(dwi_old_mask_count))
    app.debug('New mask voxel count: ' + str(dwi_new_mask_count))
    dwi_mask_overlap_image = 'dwi_mask_overlap' + iter_string + '.mif'
    run.command(['mrcalc', dwi_mask_image, new_dwi_mask_image, '-mult', dwi_mask_overlap_image])

    old_dwi_mask_image = dwi_mask_image
    dwi_mask_image = new_dwi_mask_image

    mask_overlap_count = image.statistics(dwi_mask_overlap_image,
                                          mask=dwi_mask_overlap_image).count
    app.debug('Mask overlap voxel count: ' + str(mask_overlap_count))

    new_dice_coefficient = 2.0 * mask_overlap_count / \
                           (dwi_old_mask_count + dwi_new_mask_count)

    if iteration == app.ARGS.max_iters:
      progress.done()
      app.console('Terminating due to reaching maximum %d iterations; final Dice coefficient = %f' % (iteration, new_dice_coefficient))
      app.cleanup(old_dwi_image)
      app.cleanup(old_dwi_mask_image)
      app.cleanup(old_bias_field_image)
      app.cleanup(old_tissue_sum_image)
      total_scaling_factor *= scale_multiplier
      break

    if new_dice_coefficient > app.ARGS.dice:
      progress.done()
      app.console('Exiting loop after %d iterations due to mask convergence (Dice coefficient = %f)' % (iteration, new_dice_coefficient))
      app.cleanup(old_dwi_image)
      app.cleanup(old_dwi_mask_image)
      app.cleanup(old_bias_field_image)
      app.cleanup(old_tissue_sum_image)
      total_scaling_factor *= scale_multiplier
      break

    if new_dice_coefficient < prev_dice_coefficient:
      progress.done()
      app.warn('Mask divergence at iteration %d (Dice coefficient = %f); ' % (iteration, new_dice_coefficient)
               + ' using mask from previous iteration')
      app.cleanup(dwi_image)
      app.cleanup(dwi_mask_image)
      app.cleanup(bias_field_image)
      app.cleanup(tissue_sum_image)
      dwi_image = old_dwi_image
      dwi_mask_image = old_dwi_mask_image
      bias_field_image = old_bias_field_image
      tissue_sum_image = old_tissue_sum_image
      break

    iteration += 1
    prev_dice_coefficient = new_dice_coefficient


  run.command(['mrconvert', dwi_image, path.from_user(app.ARGS.output_dwi, False)],
              mrconvert_keyval=path.from_user(app.ARGS.input, False),
              force=app.FORCE_OVERWRITE)

  if app.ARGS.output_bias:
    run.command(['mrconvert', bias_field_image, path.from_user(app.ARGS.output_bias, False)],
                mrconvert_keyval=path.from_user(app.ARGS.input, False),
                force=app.FORCE_OVERWRITE)

  if app.ARGS.output_mask:
    run.command(['mrconvert', dwi_mask_image, path.from_user(app.ARGS.output_mask, False)],
                mrconvert_keyval=path.from_user(app.ARGS.input, False),
                force=app.FORCE_OVERWRITE)

  if app.ARGS.output_scale:
    matrix.save_vector(path.from_user(app.ARGS.output_scale, False),
                       [total_scaling_factor],
                       force=app.FORCE_OVERWRITE)

  if app.ARGS.output_tissuesum:
    run.command(['mrconvert', tissue_sum_image, path.from_user(app.ARGS.output_tissuesum, False)],
                mrconvert_keyval=path.from_user(app.ARGS.input, False),
                force=app.FORCE_OVERWRITE)
