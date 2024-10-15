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

import json, os, shutil
from mrtrix3 import MRtrixError
from mrtrix3 import app, image, matrix, path, run
from .contrasts import Contrasts
from . import utils
from . import AGGREGATION_MODES, \
              DEFAULT_AFFINE_LMAX, \
              DEFAULT_AFFINE_SCALES, \
              DEFAULT_NL_LMAX, \
              DEFAULT_NL_NITER, \
              DEFAULT_NL_SCALES, \
              DEFAULT_RIGID_LMAX, \
              DEFAULT_RIGID_SCALES, \
              INITIAL_ALIGNMENT, \
              LEAVE_ONE_OUT, \
              REGISTRATION_MODES


def execute(): #pylint: disable=unused-variable

  if not app.ARGS.type in REGISTRATION_MODES:
    raise MRtrixError(f'Registration type must be one of {REGISTRATION_MODES}; provided: "{app.ARGS.type}"')
  dorigid     = 'rigid'     in app.ARGS.type
  doaffine    = 'affine'    in app.ARGS.type
  dolinear    = dorigid or doaffine
  dononlinear = 'nonlinear' in app.ARGS.type
  assert (dorigid + doaffine + dononlinear >= 1), 'FIXME: registration type not valid'


  # Reorder arguments for multi-contrast registration as after command line parsing app.ARGS.input_dir holds all but one argument
  # Also cast user-speficied options to the correct types
  input_output = app.ARGS.input_dir + [app.ARGS.template]
  n_contrasts = len(input_output) // 2
  if len(input_output) != 2 * n_contrasts:
    raise MRtrixError('Expected two arguments per contrast;'
                      f' received {len(input_output)}:'
                      f' {", ".join(input_output)}')
  app.ARGS.input_dir = []
  app.ARGS.template = []
  for i_contrast in range(n_contrasts):
    inargs = (input_output[i_contrast*2], input_output[i_contrast*2+1])
    app.ARGS.input_dir.append(app.Parser.UserInPath(inargs[0]))
    app.ARGS.template.append(app.Parser.UserOutPath(inargs[1]))
  # Perform checks that otherwise would have been done immediately after command-line parsing
  #   were it not for the inability to represent input-output pairs in the command-line interface representation
  for output_path in app.ARGS.template:
    output_path.check_output()

  if n_contrasts > 1:
    app.console('Generating population template using multi-contrast registration')

  cns = Contrasts()
  app.debug(str(cns))

  in_files = [sorted(path.all_in_dir(input_dir, dir_path=False)) for input_dir in app.ARGS.input_dir]
  if len(in_files[0]) <= 1:
    raise MRtrixError(f'Not enough images found in input directory {app.ARGS.input_dir[0]}; '
                      'more than one image is needed to generate a population template')
  if n_contrasts > 1:
    for cid in range(1, n_contrasts):
      if len(in_files[cid]) != len(in_files[0]):
        raise MRtrixError(f'Found {len(app.ARGS.input_dir[0])} images in input directory {app.ARGS.input_dir[0]} '
                          f'but {len(app.ARGS.input_dir[cid])} input images in {app.ARGS.input_dir[cid]}')
  else:
    app.console(f'Generating a population-average template from {len(in_files[0])} input images')
    if n_contrasts > 1:
      app.console(f'using {len(in_files)} contrasts for each input image')

  voxel_size = None
  if app.ARGS.voxel_size:
    voxel_size = app.ARGS.voxel_size
    if len(voxel_size) not in [1, 3]:
      raise MRtrixError('Voxel size needs to be a single or three comma-separated floating point numbers; '
                        f'received: {",".join(map(str, voxel_size))}')

  agg_measure = 'mean'
  if app.ARGS.aggregate is not None:
    if not app.ARGS.aggregate in AGGREGATION_MODES:
      raise MRtrixError(f'aggregation type must be one of {AGGREGATION_MODES}; provided: {app.ARGS.aggregate}')
    agg_measure = app.ARGS.aggregate

  agg_weights = app.ARGS.aggregation_weights
  if agg_weights is not None:
    agg_measure = 'weighted_' + agg_measure
    if agg_measure != 'weighted_mean':
      raise MRtrixError(f'aggregation weights require "-aggregate mean" option; provided: {app.ARGS.aggregate}')
    if not os.path.isfile(app.ARGS.aggregation_weights):
      raise MRtrixError(f'aggregation weights file not found: {app.ARGS.aggregation_weights}')

  initial_alignment = app.ARGS.initial_alignment
  if initial_alignment not in INITIAL_ALIGNMENT:
    raise MRtrixError(f'initial_alignment must be one of {INITIAL_ALIGNMENT}; provided: {initial_alignment}')

  linear_estimator = app.ARGS.linear_estimator
  if linear_estimator is not None and not dolinear:
    raise MRtrixError('linear_estimator specified when no linear registration is requested')

  use_masks = False
  mask_files = []
  if app.ARGS.mask_dir:
    use_masks = True
    mask_files = sorted(path.all_in_dir(app.ARGS.mask_dir, dir_path=False))
    if len(mask_files) < len(in_files[0]):
      raise MRtrixError(f'There are not enough mask images ({len(mask_files)})'
                        f' for the number of images in the input directory ({len(in_files[0])})')

  if not use_masks:
    app.warn('No masks input; use of input masks is recommended to reduce computation time and improve robustness')

  if app.ARGS.template_mask and not use_masks:
    raise MRtrixError('You cannot output a template mask because no subject masks were input using -mask_dir')

  nanmask_input = app.ARGS.nanmask
  if nanmask_input and not use_masks:
    raise MRtrixError('You cannot use NaN masking when no subject masks were input using -mask_dir')

  ins, xcontrast_xsubject_pre_postfix = utils.parse_input_files(in_files, mask_files, cns, agg_weights)

  leave_one_out = 'auto'
  if app.ARGS.leave_one_out is not None:
    leave_one_out = app.ARGS.leave_one_out
    if not leave_one_out in LEAVE_ONE_OUT:
      raise MRtrixError(f'Input to -leave_one_out not understood: {leave_one_out}')
  if leave_one_out == 'auto':
    leave_one_out = 2 < len(ins) < 15
  else:
    leave_one_out = bool(int(leave_one_out))
  if leave_one_out:
    app.console('Performing leave-one-out registration')
    # check that at sum of weights is positive for any grouping if weighted aggregation is used
    weights = [float(inp.aggregation_weight) for inp in ins if inp.aggregation_weight is not None]
    if weights and sum(weights) - max(weights) <= 0:
      raise MRtrixError('Leave-one-out registration requires positive aggregation weights in all groupings')

  noreorientation = app.ARGS.noreorientation

  do_pause_on_warn = True
  if app.ARGS.linear_no_pause:
    do_pause_on_warn = False
    if not dolinear:
      raise MRtrixError("linear option set when no linear registration is performed")

  if len(app.ARGS.template) != n_contrasts:
    raise MRtrixError(f'mismatch between number of output templates ({len(app.ARGS.template)}) '
                      'and number of contrasts ({n_contrasts})')

  if app.ARGS.transformed_dir:
    if len(app.ARGS.transformed_dir) != n_contrasts:
      raise MRtrixError(f'number of output directories specified for transformed images ({len(app.ARGS.transformed_dir)})'
                        f' does not match number of contrasts ({n_contrasts})')
    for tdir in app.ARGS.transformed_dir:
      tdir.check_output()

  if app.ARGS.linear_transformations_dir:
    if not dolinear:
      raise MRtrixError("linear option set when no linear registration is performed")

  # automatically detect SH series in each contrast
  do_fod_registration = False  # in any contrast
  cns.n_volumes = []
  cns.fod_reorientation = []
  for cid in range(n_contrasts):
    header = image.Header(ins[0].get_ims_path(False)[cid])
    image_size = header.size()
    if len(image_size) < 3 or len(image_size) > 4:
      raise MRtrixError('only 3 and 4 dimensional images can be used to build a template')
    if len(image_size) == 4:
      cns.fod_reorientation.append(header.is_sh() and not noreorientation)
      cns.n_volumes.append(image_size[3])
      do_fod_registration = do_fod_registration or cns.fod_reorientation[-1]
    else:
      cns.fod_reorientation.append(False)
      cns.n_volumes.append(0)
  if do_fod_registration:
    fod_contrasts_dirs = [app.ARGS.input_dir[cid] for cid in range(n_contrasts) if cns.fod_reorientation[cid]]
    app.console(f'SH Series detected, performing FOD registration in contrast: {", ".join(map(str, fod_contrasts_dirs))}')
  c_mrtransform_reorientation = [' -reorient_fod ' + ('yes' if cns.fod_reorientation[cid] else 'no') + ' '
                                 for cid in range(n_contrasts)]

  if nanmask_input:
    app.console('NaN-masking transformed images')

  # rigid options
  if app.ARGS.rigid_scale:
    rigid_scales = app.ARGS.rigid_scale
    if not dorigid:
      raise MRtrixError('rigid_scales option set when no rigid registration is performed')
  else:
    rigid_scales = DEFAULT_RIGID_SCALES
  if app.ARGS.rigid_lmax:
    if not dorigid:
      raise MRtrixError('-rigid_lmax option set when no rigid registration is performed')
    rigid_lmax = app.ARGS.rigid_lmax
    if do_fod_registration and len(rigid_scales) != len(rigid_lmax):
      raise MRtrixError(f'rigid_scales and rigid_lmax schedules are not equal in length: '
                        f'scales stages: {len(rigid_scales)}, lmax stages: {len(rigid_lmax)}')
  else:
    rigid_lmax = DEFAULT_RIGID_LMAX

  rigid_niter = [100] * len(rigid_scales)
  if app.ARGS.rigid_niter:
    if not dorigid:
      raise MRtrixError('-rigid_niter specified when no rigid registration is performed')
    rigid_niter = app.ARGS.rigid_niter
    if len(rigid_niter) == 1:
      rigid_niter = rigid_niter * len(rigid_scales)
    elif len(rigid_scales) != len(rigid_niter):
      raise MRtrixError(f'rigid_scales and rigid_niter schedules are not equal in length: '
                        f'scales stages: {len(rigid_scales)}, niter stages: {len(rigid_niter)}')

  # affine options
  if app.ARGS.affine_scale:
    affine_scales = app.ARGS.affine_scale
    if not doaffine:
      raise MRtrixError('-affine_scale option set when no affine registration is performed')
  else:
    affine_scales = DEFAULT_AFFINE_SCALES
  if app.ARGS.affine_lmax:
    if not doaffine:
      raise MRtrixError('affine_lmax option set when no affine registration is performed')
    affine_lmax = app.ARGS.affine_lmax
    if do_fod_registration and len(affine_scales) != len(affine_lmax):
      raise MRtrixError(f'affine_scales and affine_lmax schedules are not equal in length: '
                        f'scales stages: {len(affine_scales)}, lmax stages: {len(affine_lmax)}')
  else:
    affine_lmax = DEFAULT_AFFINE_LMAX

  affine_niter = [500] * len(affine_scales)
  if app.ARGS.affine_niter:
    if not doaffine:
      raise MRtrixError('-affine_niter specified when no affine registration is performed')
    affine_niter = app.ARGS.affine_niter
    if len(affine_niter) == 1:
      affine_niter = affine_niter * len(affine_scales)
    elif len(affine_scales) != len(affine_niter):
      raise MRtrixError(f'affine_scales and affine_niter schedules are not equal in length: '
                        f'scales stages: {len(affine_scales)}, niter stages: {len(affine_niter)}')

  linear_scales = []
  linear_lmax = []
  linear_niter = []
  linear_type = []
  if dorigid:
    linear_scales += rigid_scales
    linear_lmax += rigid_lmax
    linear_niter += rigid_niter
    linear_type += ['rigid'] * len(rigid_scales)

  if doaffine:
    linear_scales += affine_scales
    linear_lmax += affine_lmax
    linear_niter += affine_niter
    linear_type += ['affine'] * len(affine_scales)

  assert len(linear_type) == len(linear_scales)
  assert len(linear_scales) == len(linear_niter)
  if do_fod_registration:
    if len(linear_lmax) != len(linear_niter):
      mismatch = []
      if len(rigid_lmax) != len(rigid_niter):
        mismatch += [f'rigid: lmax stages: {len(rigid_lmax)}, niter stages: {len(rigid_niter)}']
      if len(affine_lmax) != len(affine_niter):
        mismatch += [f'affine: lmax stages: {len(affine_lmax)}, niter stages: {len(affine_niter)}']
      raise MRtrixError('linear registration: lmax and niter schedules are not equal in length: {", ".join(mismatch)}')
  app.console('-' * 60)
  app.console(f'initial alignment of images: {initial_alignment}')
  app.console('-' * 60)
  if n_contrasts > 1:
    for cid in range(n_contrasts):
      app.console(f'\tcontrast "{cns.suff[cid]}": {cns.names[cid]}, '
                  f'objective weight: {",".join(map(str, cns.mc_weight_initial_alignment[cid]))}')

  if dolinear:
    app.console('-' * 60)
    app.console('linear registration stages:')
    app.console('-' * 60)
    if n_contrasts > 1:
      for cid in range(n_contrasts):
        msg = f'\tcontrast "{cns.suff[cid]}": {cns.names[cid]}'
        if 'rigid' in linear_type:
          msg += f', objective weight rigid: {",".join(map(str, cns.mc_weight_rigid[cid]))}'
        if 'affine' in linear_type:
          msg += f', objective weight affine: {",".join(map(str, cns.mc_weight_affine[cid]))}'
        app.console(msg)

    if do_fod_registration:
      for istage, [tpe, scale, lmax, niter] in enumerate(zip(linear_type, linear_scales, linear_lmax, linear_niter)):
        app.console(f'({istage:02d}) {tpe.ljust(9)} scale: {scale:.4f}, niter: {niter}, lmax: {lmax}')
    else:
      for istage, [tpe, scale, niter] in enumerate(zip(linear_type, linear_scales, linear_niter)):
        app.console(f'({istage:02d}) {tpe.ljust(9)} scale: {scale:.4f}, niter: {niter}, no reorientation')

  datatype_option = ' -datatype float32'
  outofbounds_option = ' -nan'

  if not dononlinear:
    nl_scales = []
    nl_lmax = []
    nl_niter = []
    if app.ARGS.warp_dir:
      raise MRtrixError('-warp_dir specified when no nonlinear registration is performed')
  else:
    nl_scales = app.ARGS.nl_scale if app.ARGS.nl_scale else DEFAULT_NL_SCALES
    nl_niter = app.ARGS.nl_niter if app.ARGS.nl_niter else DEFAULT_NL_NITER
    nl_lmax = app.ARGS.nl_lmax if app.ARGS.nl_lmax else DEFAULT_NL_LMAX

    if len(nl_scales) != len(nl_niter):
      raise MRtrixError(f'nl_scales and nl_niter schedules are not equal in length: '
                        f'scales stages: {len(nl_scales)}, niter stages: {len(nl_niter)}')

    app.console('-' * 60)
    app.console('nonlinear registration stages:')
    app.console('-' * 60)
    if n_contrasts > 1:
      for cid in range(n_contrasts):
        app.console(f'\tcontrast "{cns.suff[cid]}": {cns.names[cid]}, '
                    f'objective weight: {",".join(map(str, cns.mc_weight_nl[cid]))}')

    if do_fod_registration:
      if len(nl_scales) != len(nl_lmax):
        raise MRtrixError(f'nl_scales and nl_lmax schedules are not equal in length: '
                          f'scales stages: {len(nl_scales)}, lmax stages: {len(nl_lmax)}')

    if do_fod_registration:
      for istage, [scale, lmax, niter] in enumerate(zip(nl_scales, nl_lmax, nl_niter)):
        app.console(f'({istage:02d}) nonlinear scale: {scale:.4f}, niter: {niter}, lmax: {lmax}')
    else:
      for istage, [scale, niter] in enumerate(zip(nl_scales, nl_niter)):
        app.console(f'({istage:02d}) nonlinear scale: {scale:.4f}, niter: {niter}, no reorientation')

  app.console('-' * 60)
  app.console('input images:')
  app.console('-' * 60)
  for inp in ins:
    app.console(f'\t{inp.info()}')

  app.activate_scratch_dir()

  for contrast in cns.suff:
    os.mkdir(f'input_transformed{contrast}')

  for contrast in cns.suff:
    os.mkdir(f'isfinite{contrast}')

  os.mkdir('linear_transforms_initial')
  os.mkdir('linear_transforms')
  for level in range(0, len(linear_scales)):
    os.mkdir(f'linear_transforms_{level:02d}')
  for level in range(0, len(nl_scales)):
    os.mkdir(f'warps_{level:02d}')

  if use_masks:
    os.mkdir('mask_transformed')
  write_log = app.VERBOSITY >= 2
  if write_log:
    os.mkdir('log')

  if initial_alignment == 'robust_mass':
    if not use_masks:
      raise MRtrixError('robust_mass initial alignment requires masks')
    os.mkdir('robust')

  if app.ARGS.copy_input:
    app.console('Copying images into scratch directory')
    for inp in ins:
      inp.cache_local()

  # Make initial template in average space using first contrast
  app.console('Generating initial template')
  input_filenames = [inp.get_ims_path(False)[0] for inp in ins]
  if voxel_size is None:
    run.command(['mraverageheader', input_filenames, 'average_header.mif',
                 '-fill',
                 '-spacing', 'mean_nearest'])
  else:
    run.command(['mraverageheader', input_filenames, '-fill', '-', '|',
                 'mrgrid', '-', 'regrid', '-voxel', ','.join(map(str, voxel_size)), 'average_header.mif'])

  # crop average space to extent defined by original masks
  if use_masks:
    progress = app.ProgressBar('Importing input masks to average space for template cropping', len(ins))
    for inp in ins:
      run.command(f'mrtransform {inp.msk_path} -interp nearest -template average_header.mif {inp.msk_transformed}')
      progress.increment()
    progress.done()
    run.command(['mrmath', [inp.msk_transformed for inp in ins], 'max', 'mask_initial.mif'])
    run.command('mrgrid average_header.mif crop -mask mask_initial.mif average_header_cropped.mif')
    run.function(os.remove, 'mask_initial.mif')
    run.function(os.remove, 'average_header.mif')
    run.function(shutil.move, 'average_header_cropped.mif', 'average_header.mif')
    progress = app.ProgressBar('Erasing temporary mask images', len(ins))
    for inp in ins:
      run.function(os.remove, inp.msk_transformed)
      progress.increment()
    progress.done()

  # create average space headers for other contrasts
  if n_contrasts > 1:
    avh3d = 'average_header3d.mif'
    avh4d = 'average_header4d.mif'
    if len(image.Header('average_header.mif').size()) == 3:
      run.command('mrconvert average_header.mif ' + avh3d)
    else:
      run.command(f'mrconvert average_header.mif -coord 3 0 -axes 0,1,2 {avh3d}')
    run.command(f'mrconvert {avh3d} -axes 0,1,2,-1 {avh4d}')
    for cid in range(n_contrasts):
      if cns.n_volumes[cid] == 0:
        run.function(utils.copy, avh3d, f'average_header{cns.suff[cid]}.mif')
      elif cns.n_volumes[cid] == 1:
        run.function(utils.copy, avh4d, f'average_header{cns.suff[cid]}.mif')
      else:
        run.command(['mrcat', [avh3d] * cns.n_volumes[cid], '-axis', '3', f'average_header{cns.suff[cid]}.mif'])
    run.function(os.remove, avh3d)
    run.function(os.remove, avh4d)
  else:
    run.function(shutil.move, 'average_header.mif', f'average_header{cns.suff[0]}.mif')

  cns.templates = [f'average_header{csuff}.mif' for csuff in cns.suff]

  if initial_alignment == 'none':
    progress = app.ProgressBar('Resampling input images to template space with no initial alignment', len(ins) * n_contrasts)
    for inp in ins:
      for cid in range(n_contrasts):
        run.command(f'mrtransform {inp.ims_path[cid]} {c_mrtransform_reorientation[cid]} -interp linear '\
                    f'-template {cns.templates[cid]} {inp.ims_transformed[cid]} {outofbounds_option} {datatype_option}')
        progress.increment()
    progress.done()

    if use_masks:
      progress = app.ProgressBar('Reslicing input masks to average header', len(ins))
      for inp in ins:
        run.command(f'mrtransform {inp.msk_path} {inp.msk_transformed} '
                    f'-interp nearest -template {cns.templates[0]} {datatype_option}')
        progress.increment()
      progress.done()

    if nanmask_input:
      utils.inplace_nan_mask([inp.ims_transformed[cid] for inp in ins for cid in range(n_contrasts)],
                             [inp.msk_transformed for inp in ins for cid in range(n_contrasts)])

    if leave_one_out:
      utils.calculate_isfinite(ins, cns)

    if not dolinear:
      for inp in ins:
        with open(os.path.join('linear_transforms_initial', f'{inp.uid}.txt'), 'w', encoding='utf-8') as fout:
          fout.write('1 0 0 0\n0 1 0 0\n0 0 1 0\n0 0 0 1\n')

    run.function(utils.copy, f'average_header{cns.suff[0]}.mif', 'average_header.mif')

  else:
    progress = app.ProgressBar('Performing initial rigid registration to template', len(ins))
    mask_option = ''
    cid = 0
    lmax_option = ' -rigid_lmax 0 ' if cns.fod_reorientation[cid] else ' -noreorientation '
    contrast_weight_option = cns.initial_alignment_weight_option
    for inp in ins:
      output_option = ' -rigid ' + os.path.join('linear_transforms_initial', f'{inp.uid}.txt')
      images = ' '.join([f'{p} {t}' for p, t in zip(inp.ims_path, cns.templates)])
      if use_masks:
        mask_option = f' -mask1 {inp.msk_path}'
        if initial_alignment == 'robust_mass':
          if not os.path.isfile('robust/template.mif'):
            if cns.n_volumes[cid] > 0:
              run.command(f'mrconvert {cns.templates[cid]} -coord 3 0 - | '
                          f'mrconvert - -axes 0,1,2 robust/template.mif')
            else:
              run.command(f'mrconvert {cns.templates[cid]} robust/template.mif')
          if n_contrasts > 1:
            cmd = ['mrcalc', inp.ims_path[cid], ','.join(map(str, cns.mc_weight_initial_alignment[cid])), '-mult']
            for cid in range(1, n_contrasts):
              cmd += [inp.ims_path[cid], ','.join(map(str, cns.mc_weight_initial_alignment[cid])), '-mult', '-add']
            contrast_weight_option = ''
            run.command(' '.join(cmd) + ' - | ' \
                        f'mrfilter - zclean -zlower 3 -zupper 3 robust/image_{inp.uid}.mif '
                        f'-maskin {inp.msk_path} -maskout robust/mask_{inp.uid}.mif')
          else:
            run.command(f'mrfilter {inp.ims_path[0]} zclean -zlower 3 -zupper 3 robust/image_{inp.uid}.mif '
                        f'-maskin {inp.msk_path} -maskout robust/mask_{inp.uid}.mif')
          images = f'robust/image_{inp.uid}.mif robust/template.mif'
          mask_option = f' -mask1 robust/mask_{inp.uid}.mif'
          lmax_option = ''

      run.command('mrregister ' + images +
                  mask_option +
                  ' -rigid_scale 1 ' +
                  ' -rigid_niter 0 ' +
                  ' -type rigid ' +
                  lmax_option +
                  contrast_weight_option +
                  ' -rigid_init_translation ' + initial_alignment.replace('robust_', '') + ' ' +
                  datatype_option +
                  output_option)
      # translate input images to centre of mass without interpolation
      transform_path = os.path.join('linear_transforms_initial', f'{inp.uid}.txt')
      for cid in range(n_contrasts):
        run.command(f'mrtransform {inp.ims_path[cid]} {c_mrtransform_reorientation[cid]} '
                    f'-linear {transform_path} '
                    f'{inp.ims_transformed[cid]}_translated.mif {datatype_option}')
      if use_masks:
        run.command(f'mrtransform {inp.msk_path} '
                    f'-linear {transform_path} '
                    f'{inp.msk_transformed}_translated.mif {datatype_option}')
      progress.increment()
    # update average space of first contrast to new extent, delete other average space images
    run.command(['mraverageheader',
                 [f'{inp.ims_transformed[cid]}_translated.mif' for inp in ins],
                 'average_header_tight.mif',
                 '-spacing', 'mean_nearest'])
    progress.done()

    if voxel_size is None:
      run.command('mrgrid average_header_tight.mif pad -uniform 10 average_header.mif', force=True)
    else:
      run.command('mrgrid average_header_tight.mif pad -uniform 10 - | '
                  'mrgrid - regrid -voxel ' + ','.join(map(str, voxel_size)) + ' average_header.mif', force=True)
    run.function(os.remove, 'average_header_tight.mif')
    for cid in range(1, n_contrasts):
      run.function(os.remove, f'average_header{cns.suff[cid]}.mif')

    if use_masks:
      # reslice masks
      progress = app.ProgressBar('Reslicing input masks to average header', len(ins))
      for inp in ins:
        run.command(f'mrtransform {inp.msk_transformed}_translated.mif {inp.msk_transformed} '
                    f'-interp nearest -template average_header.mif {datatype_option}')
        progress.increment()
      progress.done()
      # crop average space to extent defined by translated masks
      run.command(['mrmath', [inp.msk_transformed for inp in ins], 'max', 'mask_translated.mif'])
      run.command('mrgrid average_header.mif crop -mask mask_translated.mif average_header_cropped.mif')
      # pad average space to allow for deviation from initial alignment
      run.command('mrgrid average_header_cropped.mif pad -uniform 10 average_header.mif', force=True)
      run.function(os.remove, 'average_header_cropped.mif')
      # reslice masks
      progress = app.ProgressBar('Reslicing masks to new padded average header', len(ins))
      for inp in ins:
        run.command(f'mrtransform {inp.msk_transformed}_translated.mif {inp.msk_transformed} '
                    f'-interp nearest -template average_header.mif {datatype_option}',
                    force=True)
        run.function(os.remove, f'{inp.msk_transformed}_translated.mif')
        progress.increment()
      progress.done()
      run.function(os.remove, 'mask_translated.mif')

    # reslice images
    progress = app.ProgressBar('Reslicing input images to average header', len(ins) * n_contrasts)
    for cid in range(n_contrasts):
      for inp in ins:
        run.command(f'mrtransform {c_mrtransform_reorientation[cid]} {inp.ims_transformed[cid]}_translated.mif '
                    f'{inp.ims_transformed[cid]} '
                    f'-interp linear -template average_header.mif '
                    f'{outofbounds_option} {datatype_option}')
        run.function(os.remove, f'{inp.ims_transformed[cid]}_translated.mif')
        progress.increment()
    progress.done()

    if nanmask_input:
      utils.inplace_nan_mask([inp.ims_transformed[cid] for inp in ins for cid in range(n_contrasts)],
                             [inp.msk_transformed for inp in ins for cid in range(n_contrasts)])

    if leave_one_out:
      utils.calculate_isfinite(ins, cns)

  cns.templates = [f'initial_template{contrast}.mif' for contrast in cns.suff]
  for cid in range(n_contrasts):
    utils.aggregate(ins, f'initial_template{cns.suff[cid]}.mif', cid, agg_measure)
    if cns.n_volumes[cid] == 1:
      run.function(shutil.move, f'initial_template{cns.suff[cid]}.mif', 'tmp.mif')
      run.command(f'mrconvert tmp.mif initial_template{cns.suff[cid]}.mif -axes 0,1,2,-1')

  # Optimise template with linear registration
  if not dolinear:
    for inp in ins:
      run.function(utils.copy,
                   os.path.join('linear_transforms_initial', f'{inp.uid}.txt'),
                   os.path.join('linear_transforms', f'{inp.uid}.txt'))
  else:
    level = 0
    regtype = linear_type[0]

    # Preparation for drift correction;
    #   note that if initial alignment is none,
    #   calculation of the average linear transform across inputs is deferred until after the first iteration,
    #   and only then will drift correction kick in
    if not app.ARGS.linear_no_drift_correction and initial_alignment != 'none':
      app.console('Calculating average initial transform for linear drift correction')
      run.command(['transformcalc',
                   [os.path.join('linear_transforms_initial', inp.uid + '.txt') for inp in ins],
                   'average',
                   'linear_transform_average_init.txt',
                   '-quiet'])
      transform_average_driftref = matrix.load_transform('linear_transform_average_init.txt')

    def linear_msg():
      return f'Optimising template with linear registration (stage {level+1} of {len(linear_scales)}; {regtype})'
    progress = app.ProgressBar(linear_msg, len(linear_scales) * len(ins) * (1 + n_contrasts + int(use_masks)))
    for level, (regtype, scale, niter, lmax) in enumerate(zip(linear_type, linear_scales, linear_niter, linear_lmax)):
      for inp in ins:
        initialise_option = ''
        if use_masks:
          mask_option = f' -mask1 {inp.msk_path}'
        else:
          mask_option = ''
        lmax_option = ' -noreorientation'
        metric_option = ''
        init_transform_path = None if initial_alignment == 'none' else \
                              os.path.join(f'linear_transforms_{level-1:02d}' if level > 0 else 'linear_transforms_initial',
                                           f'{inp.uid}.txt')
        mrregister_log_option = ''
        output_transform_path = os.path.join(f'linear_transforms_{level:02d}', f'{inp.uid}.txt')
        linear_log_path = os.path.join('log', f'{inp.uid}{contrast[cid]}_{level}.log')
        if regtype == 'rigid':
          scale_option = f' -rigid_scale {scale}'
          niter_option = f' -rigid_niter {niter}'
          regtype_option = ' -type rigid'
          output_option = f' -rigid {output_transform_path}'
          contrast_weight_option = cns.rigid_weight_option
          initialise_option = f' -rigid_init_matrix {init_transform_path}' if init_transform_path else ''
          if do_fod_registration:
            lmax_option = f' -rigid_lmax {lmax}'
          if linear_estimator is not None:
            metric_option = f' -rigid_metric.diff.estimator {linear_estimator}'
          if app.VERBOSITY >= 2:
            mrregister_log_option = f' -info -rigid_log {linear_log_path}'
        else:
          scale_option = f' -affine_scale {scale}'
          niter_option = f' -affine_niter {niter}'
          regtype_option = ' -type affine'
          output_option = f' -affine {output_transform_path}'
          contrast_weight_option = cns.affine_weight_option
          initialise_option = f' -affine_init_matrix {init_transform_path}' if init_transform_path else ''
          if do_fod_registration:
            lmax_option = f' -affine_lmax {lmax}'
          if linear_estimator is not None:
            metric_option = f' -affine_metric.diff.estimator {linear_estimator}'
          if write_log:
            mrregister_log_option = f' -info -affine_log {linear_log_path}'

        if leave_one_out:
          tmpl = []
          for cid in range(n_contrasts):
            isfinite = f'isfinite{cns.suff[cid]}/{inp.uid}.mif'
            weight = inp.aggregation_weight if inp.aggregation_weight is not None else '1'
            # loo = (template * weighted sum - weight * this) / (weighted sum - weight)
            run.command(f'mrcalc {cns.isfinite_count[cid]} {isfinite} -sub - | '
                        f'mrcalc {cns.templates[cid]} {cns.isfinite_count[cid]} -mult {inp.ims_transformed[cid]} {weight} -mult -sub - -div '
                        f'loo_{cns.templates[cid]}',
                        force=True)
            tmpl.append(f'loo_{cns.templates[cid]}')
          images = ' '.join([p + ' ' + t for p, t in zip(inp.ims_path, tmpl)])
        else:
          images = ' '.join([p + ' ' + t for p, t in zip(inp.ims_path, cns.templates)])
        command = 'mrregister ' + images + \
                  initialise_option + \
                  mask_option + \
                  scale_option + \
                  niter_option + \
                  lmax_option + \
                  regtype_option + \
                  metric_option + \
                  datatype_option + \
                  contrast_weight_option + \
                  output_option + \
                  mrregister_log_option
        run.command(command, force=True)
        utils.check_linear_transformation(os.path.join(f'linear_transforms_{level:02d}', f'{inp.uid}.txt'),
                                          command,
                                          pause_on_warn=do_pause_on_warn)
        if leave_one_out:
          for im_temp in tmpl:
            run.function(os.remove, im_temp)
        progress.increment()

      # Here we ensure the overall template properties don't change (too much) over levels
      # The reference is the initialisation as that's used to construct the average space.
      # T_i: linear trafo for case i, i.e. template(x) = E [ image_i(T_i x) ]
      # R_i: inital trafo for case i (identity if initial alignment is none)
      # A = E[ T_i ]: average of current trafos
      # B = E[ R_i ]: average of initial trafos
      # C_i' = T_i B A^{-1}: "drift" corrected T_i
      # T_i <- C_i
      # Notes:
      #   - This approximately stabilises E[ T_i ], its' relatively close to B
      #   - Not sure whether it's preferable to stabilise E[ T_i^{-1} ]
      #   - If one subject's registration fails, this will affect the average and therefore the template which could result in instable behaviour.
      #   - The template appearance changes slightly over levels, but the template and trafos are affected in the same way so should not affect template convergence.
      if not app.ARGS.linear_no_drift_correction:
        # Where no initial alignment was performed, we instead grab the transforms from the first linear iteration
        #   and use those to form the reference for drift correction
        if initial_alignment == 'none' and level == 0:
          app.console('Calculating average transform from first iteration for linear drift correction')
          run.command(['transformcalc',
                       [os.path.join(f'linear_transforms_{level:02d}', f'{inp.uid}.txt') for inp in ins],
                       'average',
                       f'linear_transform_average_{level:02d}.txt',
                       '-quiet'])
          transform_average_driftref = matrix.load_transform(f'linear_transform_average_{level:02d}.txt')
        else:
          run.command(['transformcalc',
                       [os.path.join(f'linear_transforms_{level:02d}', f'{inp.uid}.txt') for inp in ins],
                       'average',
                       f'linear_transform_average_{level:02d}_uncorrected.txt',
                       '-quiet'],
                      force=True)
          run.command(['transformcalc',
                       f'linear_transform_average_{level:02d}_uncorrected.txt',
                       'invert',
                       f'linear_transform_average_{level:02d}_uncorrected_inv.txt',
                       '-quiet'],
                      force=True)
          transform_average_current_inv = matrix.load_transform(f'linear_transform_average_{level:02d}_uncorrected_inv.txt')
          transform_update = matrix.dot(transform_average_driftref, transform_average_current_inv)
          matrix.save_transform(f'linear_transforms_{level:02d}_drift_correction.txt', transform_update, force=True)
          if regtype == 'rigid':
            run.command(['transformcalc',
                         f'linear_transforms_{level:02d}_drift_correction.txt',
                         'rigid',
                         f'linear_transforms_{level:02d}_drift_correction.txt',
                         '-quiet'],
                        force=True)
            transform_update = matrix.load_transform(f'linear_transforms_{level:02d}_drift_correction.txt')
          for inp in ins:
            transform = matrix.load_transform(os.path.join(f'linear_transforms_{level:02d}', f'{inp.uid}.txt'))
            transform_updated = matrix.dot(transform, transform_update)
            run.function(utils.copy,
                         os.path.join(f'linear_transforms_{level:02d}', f'{inp.uid}.txt'),
                         os.path.join(f'linear_transforms_{level:02d}', f'{inp.uid}.precorrection'))
            matrix.save_transform(os.path.join(f'linear_transforms_{level:02d}', f'{inp.uid}.txt'), transform_updated, force=True)
          # compute average trafos and its properties for easier debugging
          run.command(['transformcalc',
                       [os.path.join(f'linear_transforms_{level:02d}', f'{inp.uid}.txt') for inp in ins],
                       'average',
                       f'linear_transform_average_{level:02d}.txt',
                       '-quiet'],
                      force=True)
          run.command(f'transformcalc linear_transform_average_{level:02d}.txt decompose linear_transform_average_{level:02d}.dec',
                      force=True)

      for cid in range(n_contrasts):
        for inp in ins:
          transform_path = os.path.join(f'linear_transforms_{level:02d}', f'{inp.uid}.txt')
          run.command(f'mrtransform {c_mrtransform_reorientation[cid]} {inp.ims_path[cid]}'
                      f' -template {cns.templates[cid]} '
                      f' -linear {transform_path}'
                      f' {inp.ims_transformed[cid]} {outofbounds_option} {datatype_option}',
                      force=True)
          progress.increment()

      if use_masks:
        for inp in ins:
          transform_path = os.path.join(f'linear_transforms_{level:02d}', f'{inp.uid}.txt')
          run.command(f'mrtransform {inp.msk_path} '
                      f' -template {cns.templates[0]} '
                      f' -interp nearest'
                      f' -linear {transform_path}'
                      f' {inp.msk_transformed}',
                      force=True)
          progress.increment()

      if nanmask_input:
        utils.inplace_nan_mask([inp.ims_transformed[cid] for inp in ins for cid in range(n_contrasts)],
                               [inp.msk_transformed for inp in ins for cid in range(n_contrasts)])

      if leave_one_out:
        utils.calculate_isfinite(ins, cns)

      for cid in range(n_contrasts):
        if level > 0 and app.ARGS.delete_temporary_files:
          os.remove(cns.templates[cid])
        cns.templates[cid] = f'linear_template{level:02d}{cns.suff[cid]}.mif'
        utils.aggregate(ins, cns.templates[cid], cid, agg_measure)
        if cns.n_volumes[cid] == 1:
          run.function(shutil.move, cns.templates[cid], 'tmp.mif')
          run.command(f'mrconvert tmp.mif {cns.templates[cid]} -axes 0,1,2,-1')
          run.function(os.remove, 'tmp.mif')

    for entry in os.listdir(f'linear_transforms_{level:02d}'):
      run.function(utils.copy,
                   os.path.join(f'linear_transforms_{level:02d}', entry),
                   os.path.join('linear_transforms', entry))
    progress.done()

  # Create a template mask for nl registration by taking the intersection of all transformed input masks and dilating
  if use_masks and (dononlinear or app.ARGS.template_mask):
    current_template_mask = 'init_nl_template_mask.mif'
    run.command(['mrmath', path.all_in_dir('mask_transformed'), 'min', '-', '|',
                 'maskfilter', '-', 'median', '-', '|',
                 'maskfilter', '-', 'dilate', '-npass', '5', current_template_mask],
                 force=True)

  if dononlinear:
    os.mkdir('warps')
    level = 0
    def nonlinear_msg():
      return f'Optimising template with non-linear registration (stage {level+1} of {len(nl_scales)})'
    progress = app.ProgressBar(nonlinear_msg, len(nl_scales) * len(ins))
    for level, (scale, niter, lmax) in enumerate(zip(nl_scales, nl_niter, nl_lmax)):
      for inp in ins:
        if level > 0:
          init_warp_path = os.path.join(f'warps_{level-1:02d}', f'{inp.uid}.mif')
          initialise_option = f' -nl_init {init_warp_path}'
          scale_option = ''
        else:
          scale_option = f' -nl_scale {scale}'
          init_transform_path = os.path.join('linear_transforms', f'{inp.uid}.txt')
          if not doaffine:  # rigid or no previous linear stage
            initialise_option = f' -rigid_init_matrix {init_transform_path}'
          else:
            initialise_option = f' -affine_init_matrix {init_transform_path}'

        if use_masks:
          mask_option = f' -mask1 {inp.msk_path} -mask2 {current_template_mask}'
        else:
          mask_option = ''

        if do_fod_registration:
          lmax_option = f' -nl_lmax {lmax}'
        else:
          lmax_option = ' -noreorientation'

        contrast_weight_option = cns.nl_weight_option

        if leave_one_out:
          tmpl = []
          for cid in range(n_contrasts):
            isfinite = f'isfinite{cns.suff[cid]}/{inp.uid}.mif'
            weight = inp.aggregation_weight if inp.aggregation_weight is not None else '1'
            # loo = (template * weighted sum - weight * this) / (weighted sum - weight)
            run.command(f'mrcalc {cns.isfinite_count[cid]} {isfinite} -sub - | '
                        f'mrcalc {cns.templates[cid]} {cns.isfinite_count[cid]} -mult {inp.ims_transformed[cid]} {weight} -mult -sub - -div '
                        f'loo_{cns.templates[cid]}',
                        force=True)
            tmpl.append(f'loo_{cns.templates[cid]}')
          images = ' '.join([f'{p} {t}' for p, t in zip(inp.ims_path, tmpl)])
        else:
          images = ' '.join([f'{p} {t}' for p, t in zip(inp.ims_path, cns.templates)])
        warp_full_path = os.path.join(f'warps_{level:02d}', f'{inp.uid}.mif')
        run.command(f'mrregister {images}'
                    f' -type nonlinear'
                    f' -nl_niter {nl_niter[level]}'
                    f' -nl_warp_full {warp_full_path}'
                    f' -transformed {" -transformed ".join([inp.ims_transformed[cid] for cid in range(n_contrasts)])}'
                    f' -nl_update_smooth {app.ARGS.nl_update_smooth}'
                    f' -nl_disp_smooth {app.ARGS.nl_disp_smooth}'
                    f' -nl_grad_step {app.ARGS.nl_grad_step} '
                    f' {initialise_option} {contrast_weight_option} {scale_option} {mask_option} {datatype_option} {outofbounds_option} {lmax_option}',
                    force=True)

        if use_masks:
          warp_full_path = os.path.join(f'warps_{level:02d}', f'{inp.uid}.mif')
          run.command(f'mrtransform {inp.msk_path}'
                      f' -template {cns.templates[0]}'
                      f' -warp_full {warp_full_path}'
                      f' {inp.msk_transformed}'
                      f' -interp nearest',
                      force=True)

        if leave_one_out:
          for im_temp in tmpl:
            run.function(os.remove, im_temp)

        if level > 0:
          run.function(os.remove, os.path.join(f'warps_{level-1:02d}', f'{inp.uid}.mif'))

        progress.increment(nonlinear_msg())

      if nanmask_input:
        utils.inplace_nan_mask([_inp.ims_transformed[cid] for _inp in ins for cid in range(n_contrasts)],
                               [_inp.msk_transformed for _inp in ins for cid in range(n_contrasts)])

      if leave_one_out:
        utils.calculate_isfinite(ins, cns)

      for cid in range(n_contrasts):
        if level > 0 and app.ARGS.delete_temporary_files:
          os.remove(cns.templates[cid])
        cns.templates[cid] = f'nl_template{level:02d}{cns.suff[cid]}.mif'
        utils.aggregate(ins, cns.templates[cid], cid, agg_measure)
        if cns.n_volumes[cid] == 1:
          run.function(shutil.move, cns.templates[cid], 'tmp.mif')
          run.command(f'mrconvert tmp.mif {cns.templates[cid]} -axes 0,1,2,-1')
          run.function(os.remove, 'tmp.mif')

      if use_masks:
        current_template_mask = f'nl_template_mask{level}.mif'
        run.command(['mrmath', path.all_in_dir('mask_transformed'), 'min', '-', '|',
                     'maskfilter', '-', 'median', '-', '|',
                     'maskfilter', '-', 'dilate', '-npass', '5', current_template_mask])

      if level < len(nl_scales) - 1:
        if scale < nl_scales[level + 1]:
          upsample_factor = nl_scales[level + 1] / scale
          for inp in ins:
            run.command(['mrgrid', os.path.join(f'warps_{level:02d}', f'{inp.uid}.mif'),
                        'regrid', '-scale', str(upsample_factor), 'tmp.mif'],
                        force=True)
            run.function(shutil.move, 'tmp.mif', os.path.join(f'warps_{level:02d}', f'{inp.uid}.mif'))
      else:
        for inp in ins:
          run.function(shutil.move, os.path.join(f'warps_{level:02d}', f'{inp.uid}.mif'), 'warps')
    progress.done()

  for cid in range(n_contrasts):
    run.command(['mrconvert', cns.templates[cid], app.ARGS.template[cid]],
                mrconvert_keyval='NULL',
                force=app.FORCE_OVERWRITE)

  if app.ARGS.warp_dir:
    app.ARGS.warp_dir.mkdir()
    progress = app.ProgressBar(f'Copying non-linear warps to output directory "{app.ARGS.warp_dir}"', len(ins))
    for inp in ins:
      keyval = image.Header(os.path.join('warps', f'{inp.uid}.mif')).keyval()
      keyval = dict((k, keyval[k]) for k in ('linear1', 'linear2'))
      json_path = os.path.join('warps', f'{inp.uid}.json')
      with open(json_path, 'w', encoding='utf-8') as json_file:
        json.dump(keyval, json_file)
      run.command(['mrconvert',
                   os.path.join('warps', f'{inp.uid}.mif'),
                   os.path.join(app.ARGS.warp_dir, f'{xcontrast_xsubject_pre_postfix[0]}{inp.uid}{xcontrast_xsubject_pre_postfix[1]}.mif')],
                  mrconvert_keyval=json_path,
                  force=app.FORCE_OVERWRITE)
      progress.increment()
    progress.done()

  if app.ARGS.linear_transformations_dir:
    app.ARGS.linear_transformations_dir.mkdir()
    for inp in ins:
      trafo = matrix.load_transform(os.path.join('linear_transforms', f'{inp.uid}.txt'))
      matrix.save_transform(os.path.join(app.ARGS.linear_transformations_dir,
                                         f'{xcontrast_xsubject_pre_postfix[0]}{inp.uid}{xcontrast_xsubject_pre_postfix[1]}.txt'),
                            trafo,
                            force=app.FORCE_OVERWRITE)

  if app.ARGS.transformed_dir:
    for cid, trdir in enumerate(app.ARGS.transformed_dir):
      trdir.mkdir()
      progress = app.ProgressBar(f'Copying transformed images to output directory {trdir}', len(ins))
      for inp in ins:
        run.command(['mrconvert', inp.ims_transformed[cid], os.path.join(trdir, inp.ims_filenames[cid])],
                    mrconvert_keyval=inp.get_ims_path(False)[cid],
                    force=app.FORCE_OVERWRITE)
        progress.increment()
      progress.done()

  if app.ARGS.template_mask:
    run.command(['mrconvert', current_template_mask, app.ARGS.template_mask],
                mrconvert_keyval='NULL',
                force=app.FORCE_OVERWRITE)
