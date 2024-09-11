# Copyright (c) 2008-2024 the MRtrix3 contributors.
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

DWIBIASCORRECT_MAX_ITERS = 2
LMAXES_MULTI = [4,0,0]
LMAXES_SINGLE = [4,0]
REFERENCE_INTENSITY = 1000.0
MASK_ALGOS = ['dwi2mask', 'fslbet', 'hdbet', 'mrthreshold', 'synthstrip', 'threshold']
MASK_ALGO_DEFAULT = 'threshold'
DICE_COEFF_DEFAULT = 1.0 - 1e-3

def usage(cmdline): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=no-name-in-module, import-outside-toplevel
  cmdline.set_author('Robert E. Smith (robert.smith@florey.edu.au)'
                     ' and Arshiya Sangchooli (asangchooli@student.unimelb.edu.au)')
  cmdline.set_synopsis('Perform a combination of bias field correction, intensity normalisation, and mask derivation, for DWI data')
  cmdline.add_description('DWI bias field correction,'
                          ' intensity normalisation and masking are inter-related steps,'
                          ' and errors in each may influence other steps.'
                          ' This script is designed to perform all of these steps in an integrated iterative fashion,'
                          ' with the intention of making all steps more robust.')
  cmdline.add_description('The operation of the algorithm is as follows.'
                          ' An initial mask is defined,'
                          ' either using the default dwi2mask algorithm or as provided by the user.'
                          ' Based on this mask,'
                          ' a sequence of response function estimation, '
                          'multi-shell multi-tissue CSD,'
                          ' bias field correction (using the mtnormalise command),'
                          ' and intensity normalisation is performed.'
                          ' The default dwi2mask algorithm is then re-executed on the bias-field-corrected DWI series.'
                          ' This sequence of steps is then repeated based on the revised mask,'
                          ' until either a convergence criterion or some number of maximum iterations is reached.')
  cmdline.add_description('The MRtrix3 mtnormalise command is used to estimate information'
                          ' relating to bias field and intensity normalisation.'
                          ' However its usage in this context is different to its conventional usage.'
                          ' Firstly, while the corrected ODF images are typically used directly following invocation of this command'
                          ' here the estimated bias field and scaling factors are instead used'
                          ' to apply the relevant corrections to the originating DWI data.'
                          ' Secondly, the global intensity scaling that is calculated and applied is typically based'
                          ' on achieving close to a unity sum of tissue signal fractions throughout the masked region.'
                          ' Here, it is instead the b=0 signal in CSF that forms the reference for this global intensity scaling;'
                          ' this is calculated based on the estimated CSF response function'
                          ' and the tissue-specific intensity scaling'
                          ' (this is calculated internally by mtnormalise as part of its optimisation process,'
                          ' but typically subsequently discarded in favour of a single scaling factor for all tissues)')
  cmdline.add_description('The ODFs estimated within this optimisation procedure are by default'
                          ' of lower maximal spherical harmonic degree than what would be advised for analysis.'
                          ' This is done for computational efficiency.'
                          ' This behaviour can be modified through the -lmax command-line option.')
  cmdline.add_description('By default, the optimisation procedure will terminate after only two iterations.'
                          ' This is done because it has been observed for some data / configurations that'
                          ' additional iterations can lead to unstable divergence'
                          ' and erroneous results for bias field estimation and masking.'
                          ' For other configurations,'
                          ' it may be preferable to use a greater number of iterations,'
                          ' and allow the iterative algorithm to converge to a stable solution.'
                          ' This can be controlled via the -max_iters command-line option.')
  cmdline.add_description('Within the optimisation algorithm,'
                          ' derivation of the mask may potentially be performed'
                          ' differently to a conventional mask derivation that is based on a DWI series'
                          ' (where, in many instances, it is actually only the mean b=0 image that is used).'
                          ' Here, the image corresponding to the sum of tissue signal fractions'
                          ' following spherical deconvolution / bias field correction / intensity normalisation'
                          ' is also available, '
                          ' and this can potentially be used for mask derivation.'
                          ' Available options are as follows.'
                          ' "dwi2mask": Use the MRtrix3 command dwi2mask on the bias-field-corrected DWI series'
                          ' (ie. do not use the ODF tissue sum image for mask derivation);'
                          ' the algorithm to be invoked can be controlled by the user'
                          ' via the MRtrix config file entry "Dwi2maskAlgorithm".'
                          ' "fslbet": Invoke the FSL command "bet" on the ODF tissue sum image.'
                          ' "hdbet": Invoke the HD-BET command on the ODF tissue sum image.'
                          ' "mrthreshold": Invoke the MRtrix3 command "mrthreshold" on the ODF tissue sum image,'
                          ' where an appropriate threshold value will be determined automatically'
                          ' (and some heuristic cleanup of the resulting mask will be performed).'
                          ' "synthstrip": Invoke the FreeSurfer SynthStrip method on the ODF tissue sum image.'
                          ' "threshold": Apply a fixed partial volume threshold of 0.5 to the ODF tissue sum image'
                          ' (and some heuristic cleanup of the resulting mask will be performed).')
  cmdline.add_citation('Jeurissen, B; Tournier, J-D; Dhollander, T; Connelly, A & Sijbers, J. '
                       'Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data. '
                       'NeuroImage, 2014, 103, 411-426')
  cmdline.add_citation('Raffelt, D.; Dhollander, T.; Tournier, J.-D.; Tabbara, R.; Smith, R. E.; Pierre, E. & Connelly, A. '
                       'Bias Field Correction and Intensity Normalisation for Quantitative Analysis of Apparent Fibre Density. '
                       'In Proc. ISMRM, 2017, 26, 3541')
  cmdline.add_citation('Dhollander, T.; Raffelt, D. & Connelly, A. '
                       'Unsupervised 3-tissue response function estimation from single-shell or multi-shell diffusion MR data without a co-registered T1 image. '
                       'ISMRM Workshop on Breaking the Barriers of Diffusion MRI, 2016, 5')
  cmdline.add_citation('Dhollander, T.; Tabbara, R.; Rosnarho-Tornstrand, J.; Tournier, J.-D.; Raffelt, D. & Connelly, A. '
                       'Multi-tissue log-domain intensity and inhomogeneity normalisation for quantitative apparent fibre density. '
                       'In Proc. ISMRM, 2021, 29, 2472')
  cmdline.add_argument('input',
                       type=app.Parser.ImageIn(),
                       help='The input DWI series to be corrected')
  cmdline.add_argument('output_dwi',
                       type=app.Parser.ImageOut(),
                       help='The output corrected DWI series')
  cmdline.add_argument('output_mask',
                       type=app.Parser.ImageOut(),
                       help='The output DWI mask')
  output_options = cmdline.add_argument_group('Options that modulate the outputs of the script')
  output_options.add_argument('-output_bias',
                              type=app.Parser.ImageOut(),
                              help='Export the final estimated bias field to an image')
  output_options.add_argument('-output_scale',
                              type=app.Parser.FileOut(),
                              help='Write the scaling factor applied to the DWI series to a text file')
  output_options.add_argument('-output_tissuesum',
                              type=app.Parser.ImageOut(),
                              help='Export the tissue sum image that was used to generate the final mask')
  output_options.add_argument('-reference',
                              type=app.Parser.Float(0.0),
                              default=REFERENCE_INTENSITY,
                              help='Set the target CSF b=0 intensity in the output DWI series'
                                   f' (default: {REFERENCE_INTENSITY})')
  internal_options = cmdline.add_argument_group('Options relevant to the internal optimisation procedure')
  internal_options.add_argument('-dice',
                                type=app.Parser.Float(0.0, 1.0),
                                default=DICE_COEFF_DEFAULT,
                                help='Set the Dice coefficient threshold for similarity of masks between sequential iterations'
                                     ' that will result in termination due to convergence;'
                                     f' default = {DICE_COEFF_DEFAULT}')
  internal_options.add_argument('-init_mask',
                                type=app.Parser.ImageIn(),
                                help='Provide an initial mask for the first iteration of the algorithm'
                                     ' (if not provided, the default dwi2mask algorithm will be used)')
  internal_options.add_argument('-max_iters',
                                type=app.Parser.Int(0),
                                default=DWIBIASCORRECT_MAX_ITERS,
                                metavar='count',
                                help='The maximum number of iterations (see Description);'
                                     f' default is {DWIBIASCORRECT_MAX_ITERS};'
                                     ' set to 0 to proceed until convergence')
  internal_options.add_argument('-mask_algo',
                                choices=MASK_ALGOS,
                                metavar='algorithm',
                                help='The algorithm to use for mask estimation,'
                                     ' potentially based on the ODF sum image (see Description);'
                                     f' default: {MASK_ALGO_DEFAULT}')
  internal_options.add_argument('-lmax',
                                type=app.Parser.SequenceInt(),
                                help='The maximum spherical harmonic degree for the estimated FODs (see Description);'
                                     f' defaults are "{",".join(map(str, LMAXES_MULTI))}" for multi-shell '
                                     f' and "{",".join(map(str, LMAXES_SINGLE))}" for single-shell data)')
  app.add_dwgrad_import_options(cmdline)



def execute(): #pylint: disable=unused-variable
  from mrtrix3 import CONFIG, MRtrixError  #pylint: disable=no-name-in-module, import-outside-toplevel
  from mrtrix3 import app, fsl, image, matrix, run #pylint: disable=no-name-in-module, import-outside-toplevel

  # Check user inputs
  lmax = None
  if app.ARGS.lmax:
    lmax = app.ARGS.lmax
    if any(value < 0 or value % 2 for value in lmax):
      raise MRtrixError('lmax values must be non-negative even integers')
    if len(lmax) not in [2, 3]:
      raise MRtrixError('Length of lmax vector expected to be either 2 or 3')

  # Check what masking agorithm is going to be used
  mask_algo = MASK_ALGO_DEFAULT
  if app.ARGS.mask_algo:
    mask_algo = app.ARGS.mask_algo
  elif 'DwibiasnormmaskMaskAlgorithm' in CONFIG:
    mask_algo = CONFIG['DwibiasnormmaskMaskAlgorithm']
    if not mask_algo in MASK_ALGOS:
      raise MRtrixError(f'Invalid masking algorithm selection "{mask_algo}" in MRtrix config file')
    app.console(f'"{mask_algo}" algorithm will be used for brain masking during iteration as specified in config file')
  else:
    app.console(f'Default "{MASK_ALGO_DEFAULT}" algorithm will be used for brain masking during iteration')

  # Check mask algorithm, including availability of external software if necessary
  for algo, software, command in [('fslbet', 'FSL', 'bet'), ('hdbet', 'HD-BET', 'hd-bet'), ('synthstrip', 'FreeSurfer', 'mri_synthstrip')]:
    if mask_algo == algo and not shutil.which(command):
      raise MRtrixError(f'{software} command "{command}" not found; cannot use for internal mask calculations')

  app.activate_scratch_dir()
  run.command(['mrconvert', app.ARGS.input, 'input.mif']
              + app.dwgrad_import_options(),
              preserve_pipes=True)
  if app.ARGS.init_mask:
    run.command(['mrconvert', app.ARGS.init_mask, 'dwi_mask_init.mif',
                 '-datatype', 'bit'],
                 preserve_pipes=True)



  # Check inputs
  # Make sure it's actually a DWI that's been passed
  dwi_header = image.Header('input.mif')
  if len(dwi_header.size()) != 4:
    raise MRtrixError('Input image must be a 4D image')
  if 'dw_scheme' not in dwi_header.keyval():
    raise MRtrixError('No valid DW gradient scheme provided or present in image header')
  dwscheme_rows = len(dwi_header.keyval()['dw_scheme'])
  if dwscheme_rows != dwi_header.size()[3]:
    raise MRtrixError(f'DW gradient scheme contains different number of entries'
                      f' ({dwscheme_rows})'
                      f' to number of volumes in DWIs'
                      f' ({dwi_header.size()[3]})')

  # Determine whether we are working with single-shell or multi-shell data
  bvalues = [
      int(round(float(value)))
      for value in image.mrinfo('input.mif', 'shell_bvalues') \
                                .strip().split()]
  multishell = len(bvalues) > 2
  if lmax is None:
    lmax = LMAXES_MULTI if multishell else LMAXES_SINGLE
  elif len(lmax) == 3 and not multishell:
    raise MRtrixError('User specified 3 lmax values for three-tissue decomposition, '
                      'but input DWI is not multi-shell')
  lmax_option = ' -lmax ' + ','.join(map(str, lmax))

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
    run.command(['dwi2mask', CONFIG['Dwi2maskAlgorithm'], 'input.mif', 'dwi_mask_init.mif'])

  # Combined RF estimation / CSD / mtnormalise / mask revision
  class Tissue(object): #pylint: disable=useless-object-inheritance
    def __init__(self, name, index):
      self.name = name
      iter_string = '_iter' + str(index)
      self.tissue_rf = 'response_' + name + iter_string + '.txt'
      self.fod_init = 'FODinit_' + name + iter_string + '.mif'
      self.fod_norm = 'FODnorm_' + name + iter_string + '.mif'

  iteration_message = f'a maximum of {app.ARGS.max_iters}' \
                      if app.ARGS.max_iters \
                      else 'no limit on number of '
  app.debug('Commencing iterative DWI bias field correction and brain masking '
            f'with {iteration_message} iterations')

  dwi_image = 'input.mif'
  dwi_mask_image = 'dwi_mask_init.mif'
  bias_field_image = None
  tissue_sum_image = None
  iteration = 0
  step = 'initialisation'
  prev_dice_coefficient = None
  total_scaling_factor = 1.0

  def msg():
    return f'Iteration {iteration}; {step} step; previous Dice coefficient {prev_dice_coefficient}'
  progress = app.ProgressBar(msg)

  iteration = 1
  while True:
    iter_string = '_iter' + str(iteration)
    tissues = [Tissue('WM', iteration),
               Tissue('GM', iteration),
               Tissue('CSF', iteration)]

    step = 'dwi2response'
    progress.increment()
    run.command(f'dwi2response dhollander {dwi_image} -mask {dwi_mask_image} '
                + ' '.join(tissue.tissue_rf for tissue in tissues))


    # Immediately remove GM if we can't deal with it
    if not multishell:
      app.cleanup(tissues[1].tissue_rf)
      tissues = tissues[::2]

    step = 'dwi2fod'
    progress.increment()
    app.debug('Performing CSD with lmax values: ' + ','.join(map(str, lmax)))
    run.command(f'dwi2fod msmt_csd {dwi_image} {lmax_option} '
                + ' '.join(tissue.tissue_rf + ' ' + tissue.fod_init for tissue in tissues))

    step = 'maskfilter'
    progress.increment()
    eroded_mask = os.path.splitext(dwi_mask_image)[0] + '_eroded.mif'
    run.command(f'maskfilter {dwi_mask_image} erode {eroded_mask}')

    step = 'mtnormalise'
    progress.increment()
    old_bias_field_image = bias_field_image
    bias_field_image = 'field' + iter_string + '.mif'
    factors_path = 'factors' + iter_string + '.txt'

    run.command(f'mtnormalise -balanced -mask {eroded_mask} -check_norm {bias_field_image} -check_factors {factors_path} '
                + ' '.join(tissue.fod_init + ' ' + tissue.fod_norm
                           for tissue in tissues))
    app.cleanup([tissue.fod_init for tissue in tissues])
    app.cleanup(eroded_mask)

    app.debug(f'Iteration {iteration}, applying estimated bias field and appropiate scaling factor...')
    csf_rf = matrix.load_matrix(tissues[-1].tissue_rf)
    csf_rf_bzero_lzero = csf_rf[0][0]
    app.cleanup([tissue.tissue_rf for tissue in tissues])
    balance_factors = matrix.load_vector(factors_path)
    csf_balance_factor = balance_factors[-1]
    app.cleanup(factors_path)
    scale_multiplier = (app.ARGS.reference * math.sqrt(4.0*math.pi)) / \
                       (csf_rf_bzero_lzero / csf_balance_factor)
    new_dwi_image = f'dwi{iter_string}.mif'
    run.command(f'mrcalc {dwi_image} {bias_field_image} -div {scale_multiplier} -mult {new_dwi_image}')

    old_dwi_image = dwi_image
    dwi_image = new_dwi_image

    old_tissue_sum_image = tissue_sum_image
    tissue_sum_image = f'tissue_sum{iter_string}.mif'

    app.debug(f'Iteration {iteration}, revising brain mask...')
    step = 'masking'
    progress.increment()

    run.command(f'mrconvert {tissues[0].fod_norm} -coord 3 0 - | ' \
                f'mrmath - {" ".join(tissue.fod_norm for tissue in tissues[1:])} sum - | '
                f'mrcalc - {math.sqrt(4.0 * math.pi)} -mult {tissue_sum_image}')
    app.cleanup([tissue.fod_norm for tissue in tissues])

    new_dwi_mask_image = f'dwi_mask{iter_string}.mif'
    tissue_sum_image_nii = None
    new_dwi_mask_image_nii = None
    if mask_algo in ['fslbet', 'hdbet', 'synthstrip']:
      tissue_sum_image_nii = f'{os.path.splitext(tissue_sum_image)[0]}.nii'
      run.command(f'mrconvert {tissue_sum_image} {tissue_sum_image_nii}')
      new_dwi_mask_image_nii = f'{os.path.splitext(new_dwi_mask_image)[0]}.nii'
    if mask_algo == 'dwi2mask':
      run.command(['dwi2mask', CONFIG.get('Dwi2maskAlgorithm', 'legacy'), new_dwi_image, new_dwi_mask_image])
    elif mask_algo == 'fslbet':
      run.command(f'bet {tissue_sum_image_nii} {new_dwi_mask_image_nii} -R -m')
      app.cleanup(fsl.find_image(os.path.splitext(new_dwi_mask_image_nii)[0]))
      new_dwi_mask_image_nii = fsl.find_image(os.path.splitext(new_dwi_mask_image_nii)[0] + '_mask')
      run.command(f'mrcalc {new_dwi_mask_image_nii} input_pos_mask.mif -mult {new_dwi_mask_image}')
    elif mask_algo == 'hdbet':
      try:
        run.command(f'hd-bet -i {tissue_sum_image_nii}')
      except run.MRtrixCmdError as e_gpu:
        try:
          run.command(f'hd-bet -i {tissue_sum_image_nii} -device cpu -mode fast -tta 0')
        except run.MRtrixCmdError as e_cpu:
          raise run.MRtrixCmdError('hd-bet', 1, e_gpu.stdout + e_cpu.stdout, e_gpu.stderr + e_cpu.stderr)
      new_dwi_mask_image_nii = os.path.splitext(tissue_sum_image)[0] + '_bet_mask.nii.gz'
      run.command(f'mrcalc {new_dwi_mask_image_nii} input_pos_mask.mif -mult {new_dwi_mask_image}')
    elif mask_algo in ['mrthreshold', 'threshold']:
      mrthreshold_abs_option = ' -abs 0.5' if mask_algo == 'threshold' else ''
      run.command(f'mrthreshold {tissue_sum_image} {mrthreshold_abs_option} - |'
                  f' maskfilter - connect -largest - |'
                  f' mrcalc 1 - -sub - -datatype bit |'
                  f' maskfilter - connect -largest - |'
                  f' mrcalc 1 - -sub - -datatype bit |'
                  f' maskfilter - clean - |'
                  f' mrcalc - input_pos_mask.mif -mult {new_dwi_mask_image} -datatype bit')
    elif mask_algo == 'synthstrip':
      run.command(f'mri_synthstrip -i {tissue_sum_image_nii} --mask {new_dwi_mask_image_nii}')
      run.command(f'mrcalc {new_dwi_mask_image_nii} input_pos_mask.mif -mult {new_dwi_mask_image}')
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
    app.debug(f'Old mask voxel count: {dwi_old_mask_count}')
    app.debug(f'New mask voxel count: {dwi_new_mask_count}')
    dwi_mask_overlap_image = f'dwi_mask_overlap{iter_string}.mif'
    run.command(f'mrcalc {dwi_mask_image} {new_dwi_mask_image} -mult {dwi_mask_overlap_image} -datatype bit')

    old_dwi_mask_image = dwi_mask_image
    dwi_mask_image = new_dwi_mask_image

    mask_overlap_count = image.statistics(dwi_mask_overlap_image,
                                          mask=dwi_mask_overlap_image).count
    app.debug(f'Mask overlap voxel count: {mask_overlap_count}')

    new_dice_coefficient = 2.0 * mask_overlap_count / \
                           (dwi_old_mask_count + dwi_new_mask_count)

    if iteration == app.ARGS.max_iters:
      progress.done()
      app.console(f'Terminating due to reaching maximum {iteration} iterations; '
                  f'final Dice coefficient = {new_dice_coefficient}')
      app.cleanup(old_dwi_image)
      app.cleanup(old_dwi_mask_image)
      app.cleanup(old_bias_field_image)
      app.cleanup(old_tissue_sum_image)
      total_scaling_factor *= scale_multiplier
      break

    if new_dice_coefficient > app.ARGS.dice:
      progress.done()
      app.console(f'Exiting loop after {iteration} iterations due to mask convergence '
                  f'(Dice coefficient = {new_dice_coefficient})')
      app.cleanup(old_dwi_image)
      app.cleanup(old_dwi_mask_image)
      app.cleanup(old_bias_field_image)
      app.cleanup(old_tissue_sum_image)
      total_scaling_factor *= scale_multiplier
      break

    if prev_dice_coefficient is not None and new_dice_coefficient < prev_dice_coefficient:
      progress.done()
      app.warn(f'Mask divergence at iteration {iteration} '
               f'(Dice coefficient = {new_dice_coefficient}); '
               f' using mask from previous iteration')
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


  run.command(['mrconvert', dwi_image, app.ARGS.output_dwi],
              mrconvert_keyval=app.ARGS.input,
              preserve_pipes=True,
              force=app.FORCE_OVERWRITE)

  if app.ARGS.output_bias:
    run.command(['mrconvert', bias_field_image, app.ARGS.output_bias],
                mrconvert_keyval=app.ARGS.input,
                preserve_pipes=True,
                force=app.FORCE_OVERWRITE)

  if app.ARGS.output_mask:
    run.command(['mrconvert', dwi_mask_image, app.ARGS.output_mask],
                mrconvert_keyval=app.ARGS.input,
                preserve_pipes=True,
                force=app.FORCE_OVERWRITE)

  if app.ARGS.output_scale:
    matrix.save_vector(app.ARGS.output_scale,
                       [total_scaling_factor],
                       force=app.FORCE_OVERWRITE)

  if app.ARGS.output_tissuesum:
    run.command(['mrconvert', tissue_sum_image, app.ARGS.output_tissuesum],
                mrconvert_keyval=app.ARGS.input,
                preserve_pipes=True,
                force=app.FORCE_OVERWRITE)
