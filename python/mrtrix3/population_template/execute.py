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

import json, os, shlex, shutil
from mrtrix3 import EXE_LIST, MRtrixError
from mrtrix3 import app, image, matrix, path, run
from . import AGGREGATION_MODES, REGISTRATION_MODES
from . import DEFAULT_AFFINE_LMAX, DEFAULT_AFFINE_SCALES
from . import DEFAULT_RIGID_LMAX, DEFAULT_RIGID_SCALES
from . import DEFAULT_NL_LMAX, DEFAULT_NL_NITER, DEFAULT_NL_SCALES
from .contrasts import Contrasts
from .utils import aggregate, calculate_isfinite, check_linear_transformation, copy, inplace_nan_mask, parse_input_files, relpath

def execute(): #pylint: disable=unused-variable

  expected_commands = ['mrgrid', 'mrregister', 'mrtransform', 'mraverageheader', 'mrconvert', 'mrmath', 'transformcalc', 'mrfilter']
  for cmd in expected_commands:
    if cmd not in EXE_LIST :
      raise MRtrixError("Could not find " + cmd + " in bin/. Binary commands not compiled?")

  if not app.ARGS.type in REGISTRATION_MODES:
    raise MRtrixError("registration type must be one of %s. provided: %s" % (str(REGISTRATION_MODES), app.ARGS.type))
  dorigid     = "rigid"     in app.ARGS.type
  doaffine    = "affine"    in app.ARGS.type
  dolinear    = dorigid or doaffine
  dononlinear = "nonlinear" in app.ARGS.type
  assert (dorigid + doaffine + dononlinear >= 1), "FIXME: registration type not valid"

  input_output = app.ARGS.input_dir + [app.ARGS.template]
  n_contrasts = len(input_output) // 2
  if len(input_output) != 2 * n_contrasts:
    raise MRtrixError('expected two arguments per contrast, received %i: %s' % (len(input_output), ', '.join(input_output)))
  if n_contrasts > 1:
    app.console('Generating population template using multi-contrast registration')

  # reorder arguments for multi-contrast registration as after command line parsing app.ARGS.input_dir holds all but one argument
  app.ARGS.input_dir = []
  app.ARGS.template = []
  for i_contrast in range(n_contrasts):
    inargs = (input_output[i_contrast*2], input_output[i_contrast*2+1])
    if not os.path.isdir(inargs[0]):
      raise MRtrixError('input directory %s not found' % inargs[0])
    app.ARGS.input_dir.append(relpath(inargs[0]))
    app.ARGS.template.append(relpath(inargs[1]))

  cns = Contrasts()
  app.debug(str(cns))

  in_files = [sorted(path.all_in_dir(input_dir, dir_path=False)) for input_dir in app.ARGS.input_dir]
  if len(in_files[0]) <= 1:
    raise MRtrixError('Not enough images found in input directory ' + app.ARGS.input_dir[0] +
                      '. More than one image is needed to generate a population template')
  if n_contrasts > 1:
    for cid in range(1, n_contrasts):
      if len(in_files[cid]) != len(in_files[0]):
        raise MRtrixError('Found %i images in input directory %s ' % (len(app.ARGS.input_dir[0]), app.ARGS.input_dir[0]) +
                          'but %i input images in %s.' % (len(app.ARGS.input_dir[cid]), app.ARGS.input_dir[cid]))
  else:
    app.console('Generating a population-average template from ' + str(len(in_files[0])) + ' input images')
    if n_contrasts > 1:
      app.console('using ' + str(len(in_files)) + ' contrasts for each input image')

  voxel_size = None
  if app.ARGS.voxel_size:
    voxel_size = app.ARGS.voxel_size.split(',')
    if len(voxel_size) == 1:
      voxel_size = voxel_size * 3
    try:
      if len(voxel_size) != 3:
        raise ValueError
      [float(v) for v in voxel_size]  #pylint: disable=expression-not-assigned
    except ValueError as exception:
      raise MRtrixError('voxel size needs to be a single or three comma-separated floating point numbers; received: ' + str(app.ARGS.voxel_size)) from exception

  agg_measure = 'mean'
  if app.ARGS.aggregate is not None:
    if not app.ARGS.aggregate in AGGREGATION_MODES:
      raise MRtrixError("aggregation type must be one of %s. provided: %s" % (str(AGGREGATION_MODES), app.ARGS.aggregate))
    agg_measure = app.ARGS.aggregate

  agg_weights = app.ARGS.aggregation_weights
  if agg_weights is not None:
    agg_measure = "weighted_" + agg_measure
    if agg_measure != 'weighted_mean':
      raise MRtrixError ("aggregation weights require '-aggregate mean' option. provided: %s" % (app.ARGS.aggregate))
    if not os.path.isfile(app.ARGS.aggregation_weights):
      raise MRtrixError("aggregation weights file not found: %s" % app.ARGS.aggregation_weights)

  initial_alignment = app.ARGS.initial_alignment
  if initial_alignment not in ["mass", "robust_mass", "geometric", "none"]:
    raise MRtrixError('initial_alignment must be one of ' + " ".join(["mass", "robust_mass", "geometric", "none"]) + " provided: " + str(initial_alignment))

  linear_estimator = app.ARGS.linear_estimator
  if linear_estimator and not linear_estimator.lower() == 'none':
    if not dolinear:
      raise MRtrixError('linear_estimator specified when no linear registration is requested')
    if linear_estimator not in ["l1", "l2", "lp"]:
      raise MRtrixError('linear_estimator must be one of ' + " ".join(["l1", "l2", "lp"]) + " provided: " + str(linear_estimator))

  use_masks = False
  mask_files = []
  if app.ARGS.mask_dir:
    use_masks = True
    app.ARGS.mask_dir = relpath(app.ARGS.mask_dir)
    if not os.path.isdir(app.ARGS.mask_dir):
      raise MRtrixError('mask directory not found')
    mask_files = sorted(path.all_in_dir(app.ARGS.mask_dir, dir_path=False))
    if len(mask_files) < len(in_files[0]):
      raise MRtrixError('there are not enough mask images for the number of images in the input directory')

  if not use_masks:
    app.warn('no masks input. Use input masks to reduce computation time and improve robustness')

  if app.ARGS.template_mask and not use_masks:
    raise MRtrixError('you cannot output a template mask because no subject masks were input using -mask_dir')

  nanmask_input = app.ARGS.nanmask
  if nanmask_input and not use_masks:
    raise MRtrixError('you cannot use NaN masking when no subject masks were input using -mask_dir')

  ins, xcontrast_xsubject_pre_postfix = parse_input_files(in_files, mask_files, cns, agg_weights)

  leave_one_out = 'auto'
  if app.ARGS.leave_one_out is not None:
    leave_one_out = app.ARGS.leave_one_out
    if not leave_one_out in ['0', '1', 'auto']:
      raise MRtrixError('leave_one_out not understood: ' + str(leave_one_out))
  if leave_one_out == 'auto':
    leave_one_out = 2 < len(ins) < 15
  else:
    leave_one_out = bool(int(leave_one_out))
  if leave_one_out:
    app.console('performing leave-one-out registration')
    # check that at sum of weights is positive for any grouping if weighted aggregation is used
    weights = [float(inp.aggregation_weight) for inp in ins if inp.aggregation_weight is not None]
    if weights and sum(weights) - max(weights) <= 0:
      raise MRtrixError('leave-one-out registration requires positive aggregation weights in all groupings')

  noreorientation = app.ARGS.noreorientation

  do_pause_on_warn = True
  if app.ARGS.linear_no_pause:
    do_pause_on_warn = False
    if not dolinear:
      raise MRtrixError("linear option set when no linear registration is performed")

  if len(app.ARGS.template) != n_contrasts:
    raise MRtrixError('mismatch between number of output templates (%i) ' % len(app.ARGS.template) +
                      'and number of contrasts (%i)' % n_contrasts)
  for templ in app.ARGS.template:
    app.check_output_path(templ)

  if app.ARGS.warp_dir:
    app.ARGS.warp_dir = relpath(app.ARGS.warp_dir)
    app.check_output_path(app.ARGS.warp_dir)

  if app.ARGS.transformed_dir:
    app.ARGS.transformed_dir = [relpath(d) for d in app.ARGS.transformed_dir.split(',')]
    if len(app.ARGS.transformed_dir) != n_contrasts:
      raise MRtrixError('require multiple comma separated transformed directories if multi-contrast registration is used')
    for tdir in app.ARGS.transformed_dir:
      app.check_output_path(tdir)

  if app.ARGS.linear_transformations_dir:
    if not dolinear:
      raise MRtrixError("linear option set when no linear registration is performed")
    app.ARGS.linear_transformations_dir = relpath(app.ARGS.linear_transformations_dir)
    app.check_output_path(app.ARGS.linear_transformations_dir)

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
    app.console("SH Series detected, performing FOD registration in contrast: " +
                ', '.join(app.ARGS.input_dir[cid] for cid in range(n_contrasts) if cns.fod_reorientation[cid]))
  c_mrtransform_reorientation = [' -reorient_fod ' + ('yes' if cns.fod_reorientation[cid] else 'no') + ' '
                                 for cid in range(n_contrasts)]

  if nanmask_input:
    app.console("NaN masking transformed images")

  # rigid options
  if app.ARGS.rigid_scale:
    rigid_scales = [float(x) for x in app.ARGS.rigid_scale.split(',')]
    if not dorigid:
      raise MRtrixError("rigid_scales option set when no rigid registration is performed")
  else:
    rigid_scales = DEFAULT_RIGID_SCALES
  if app.ARGS.rigid_lmax:
    if not dorigid:
      raise MRtrixError("rigid_lmax option set when no rigid registration is performed")
    rigid_lmax = [int(x) for x in app.ARGS.rigid_lmax.split(',')]
    if do_fod_registration and len(rigid_scales) != len(rigid_lmax):
      raise MRtrixError('rigid_scales and rigid_lmax schedules are not equal in length: scales stages: %s, lmax stages: %s' % (len(rigid_scales), len(rigid_lmax)))
  else:
    rigid_lmax = DEFAULT_RIGID_LMAX

  rigid_niter = [100] * len(rigid_scales)
  if app.ARGS.rigid_niter:
    if not dorigid:
      raise MRtrixError("rigid_niter specified when no rigid registration is performed")
    rigid_niter = [int(x) for x in app.ARGS.rigid_niter.split(',')]
    if len(rigid_niter) == 1:
      rigid_niter = rigid_niter * len(rigid_scales)
    elif len(rigid_scales) != len(rigid_niter):
      raise MRtrixError('rigid_scales and rigid_niter schedules are not equal in length: scales stages: %s, niter stages: %s' % (len(rigid_scales), len(rigid_niter)))

  # affine options
  if app.ARGS.affine_scale:
    affine_scales = [float(x) for x in app.ARGS.affine_scale.split(',')]
    if not doaffine:
      raise MRtrixError("affine_scale option set when no affine registration is performed")
  else:
    affine_scales = DEFAULT_AFFINE_SCALES
  if app.ARGS.affine_lmax:
    if not doaffine:
      raise MRtrixError("affine_lmax option set when no affine registration is performed")
    affine_lmax = [int(x) for x in app.ARGS.affine_lmax.split(',')]
    if do_fod_registration and len(affine_scales) != len(affine_lmax):
      raise MRtrixError('affine_scales and affine_lmax schedules are not equal in length: scales stages: %s, lmax stages: %s' % (len(affine_scales), len(affine_lmax)))
  else:
    affine_lmax = DEFAULT_AFFINE_LMAX

  affine_niter = [500] * len(affine_scales)
  if app.ARGS.affine_niter:
    if not doaffine:
      raise MRtrixError("affine_niter specified when no affine registration is performed")
    affine_niter = [int(x) for x in app.ARGS.affine_niter.split(',')]
    if len(affine_niter) == 1:
      affine_niter = affine_niter * len(affine_scales)
    elif len(affine_scales) != len(affine_niter):
      raise MRtrixError('affine_scales and affine_niter schedules are not equal in length: scales stages: %s, niter stages: %s' % (len(affine_scales), len(affine_niter)))

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
        mismatch += ['rigid: lmax stages: %s, niter stages: %s' % (len(rigid_lmax), len(rigid_niter))]
      if len(affine_lmax) != len(affine_niter):
        mismatch += ['affine: lmax stages: %s, niter stages: %s' % (len(affine_lmax), len(affine_niter))]
      raise MRtrixError('linear registration: lmax and niter schedules are not equal in length: %s' % (', '.join(mismatch)))
  app.console('-' * 60)
  app.console('initial alignment of images: %s' % initial_alignment)
  app.console('-' * 60)
  if n_contrasts > 1:
    for cid in range(n_contrasts):
      app.console('\tcontrast "%s": %s, ' % (cns.suff[cid], cns.names[cid]) +
                  'objective weight: %s' % cns.mc_weight_initial_alignment[cid])

  if dolinear:
    app.console('-' * 60)
    app.console('linear registration stages:')
    app.console('-' * 60)
    if n_contrasts > 1:
      for cid in range(n_contrasts):
        msg = '\tcontrast "%s": %s' % (cns.suff[cid], cns.names[cid])
        if 'rigid' in linear_type:
          msg += ', objective weight rigid: %s' % cns.mc_weight_rigid[cid]
        if 'affine' in linear_type:
          msg += ', objective weight affine: %s' % cns.mc_weight_affine[cid]
        app.console(msg)

    if do_fod_registration:
      for istage, [tpe, scale, lmax, niter] in enumerate(zip(linear_type, linear_scales, linear_lmax, linear_niter)):
        app.console('(%02i) %s scale: %.4f, niter: %i, lmax: %i' % (istage, tpe.ljust(9), scale, niter, lmax))
    else:
      for istage, [tpe, scale, niter] in enumerate(zip(linear_type, linear_scales, linear_niter)):
        app.console('(%02i) %s scale: %.4f, niter: %i, no reorientation' % (istage, tpe.ljust(9), scale, niter))

  datatype_option = ' -datatype float32'
  outofbounds_option = ' -nan'

  if not dononlinear:
    nl_scales = []
    nl_lmax = []
    nl_niter = []
    if app.ARGS.warp_dir:
      raise MRtrixError('warp_dir specified when no nonlinear registration is performed')
  else:
    nl_scales = [float(x) for x in app.ARGS.nl_scale.split(',')] if app.ARGS.nl_scale else DEFAULT_NL_SCALES
    nl_niter = [int(x) for x in app.ARGS.nl_niter.split(',')] if app.ARGS.nl_niter else DEFAULT_NL_NITER
    nl_lmax = [int(x) for x in app.ARGS.nl_lmax.split(',')] if app.ARGS.nl_lmax else DEFAULT_NL_LMAX

    if len(nl_scales) != len(nl_niter):
      raise MRtrixError('nl_scales and nl_niter schedules are not equal in length: scales stages: %s, niter stages: %s' % (len(nl_scales), len(nl_niter)))

    app.console('-' * 60)
    app.console('nonlinear registration stages:')
    app.console('-' * 60)
    if n_contrasts > 1:
      for cid in range(n_contrasts):
        app.console('\tcontrast "%s": %s, objective weight: %s' % (cns.suff[cid], cns.names[cid], cns.mc_weight_nl[cid]))

    if do_fod_registration:
      if len(nl_scales) != len(nl_lmax):
        raise MRtrixError('nl_scales and nl_lmax schedules are not equal in length: scales stages: %s, lmax stages: %s' % (len(nl_scales), len(nl_lmax)))

    if do_fod_registration:
      for istage, [scale, lmax, niter] in enumerate(zip(nl_scales, nl_lmax, nl_niter)):
        app.console('(%02i) nonlinear scale: %.4f, niter: %i, lmax: %i' % (istage, scale, niter, lmax))
    else:
      for istage, [scale, niter] in enumerate(zip(nl_scales, nl_niter)):
        app.console('(%02i) nonlinear scale: %.4f, niter: %i, no reorientation' % (istage, scale, niter))

  app.console('-' * 60)
  app.console('input images:')
  app.console('-' * 60)
  for inp in ins:
    app.console('\t' + inp.info())

  app.make_scratch_dir()
  app.goto_scratch_dir()

  for contrast in cns.suff:
    path.make_dir('input_transformed' + contrast)

  for contrast in cns.suff:
    path.make_dir('isfinite' + contrast)

  path.make_dir('linear_transforms_initial')
  path.make_dir('linear_transforms')
  for level in range(0, len(linear_scales)):
    path.make_dir('linear_transforms_%02i' % level)
  for level in range(0, len(nl_scales)):
    path.make_dir('warps_%02i' % level)

  if use_masks:
    path.make_dir('mask_transformed')
  write_log = (app.VERBOSITY >= 2)
  if write_log:
    path.make_dir('log')

  if initial_alignment == 'robust_mass':
    if not use_masks:
      raise MRtrixError('robust_mass initial alignment requires masks')
    path.make_dir('robust')

  if app.ARGS.copy_input:
    app.console('Copying images into scratch directory')
    for inp in ins:
      inp.cache_local()

  # Make initial template in average space using first contrast
  app.console('Generating initial template')
  input_filenames = [inp.get_ims_path(False)[0] for inp in ins]
  if voxel_size is None:
    run.command(['mraverageheader', input_filenames, 'average_header.mif', '-fill'])
  else:
    run.command(['mraverageheader', '-fill', input_filenames, '-', '|',
                 'mrgrid', '-', 'regrid', '-voxel', ','.join(map(str, voxel_size)), 'average_header.mif'])

  # crop average space to extent defined by original masks
  if use_masks:
    progress = app.ProgressBar('Importing input masks to average space for template cropping', len(ins))
    for inp in ins:
      run.command('mrtransform ' + inp.msk_path + ' -interp nearest -template average_header.mif ' + inp.msk_transformed)
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
      run.command('mrconvert average_header.mif -coord 3 0 -axes 0,1,2 ' + avh3d)
    run.command('mrconvert ' + avh3d + ' -axes 0,1,2,-1 ' + avh4d)
    for cid in range(n_contrasts):
      if cns.n_volumes[cid] == 0:
        run.function(copy, avh3d, 'average_header' + cns.suff[cid] + '.mif')
      elif cns.n_volumes[cid] == 1:
        run.function(copy, avh4d, 'average_header' + cns.suff[cid] + '.mif')
      else:
        run.command(['mrcat', [avh3d] * cns.n_volumes[cid], '-axis', '3', 'average_header' + cns.suff[cid] + '.mif'])
    run.function(os.remove, avh3d)
    run.function(os.remove, avh4d)
  else:
    run.function(shutil.move, 'average_header.mif', 'average_header' + cns.suff[0] + '.mif')

  cns.templates = ['average_header' + csuff + '.mif' for csuff in cns.suff]

  if initial_alignment == 'none':
    progress = app.ProgressBar('Resampling input images to template space with no initial alignment', len(ins) * n_contrasts)
    for inp in ins:
      for cid in range(n_contrasts):
        run.command('mrtransform ' + inp.ims_path[cid] + c_mrtransform_reorientation[cid] + ' -interp linear ' +
                    '-template ' + cns.templates[cid] + ' ' + inp.ims_transformed[cid] +
                    outofbounds_option +
                    datatype_option)
        progress.increment()
    progress.done()

    if use_masks:
      progress = app.ProgressBar('Reslicing input masks to average header', len(ins))
      for inp in ins:
        run.command('mrtransform ' + inp.msk_path + ' ' + inp.msk_transformed + ' ' +
                    '-interp nearest -template ' + cns.templates[0] + ' ' +
                    datatype_option)
        progress.increment()
      progress.done()

    if nanmask_input:
      inplace_nan_mask([inp.ims_transformed[cid] for inp in ins for cid in range(n_contrasts)],
                       [inp.msk_transformed for inp in ins for cid in range(n_contrasts)])

    if leave_one_out:
      calculate_isfinite(ins, cns)

    if not dolinear:
      for inp in ins:
        with open(os.path.join('linear_transforms_initial', inp.uid + '.txt'), 'w', encoding='utf-8') as fout:
          fout.write('1 0 0 0\n0 1 0 0\n0 0 1 0\n0 0 0 1\n')

    run.function(copy, 'average_header' + cns.suff[0] + '.mif', 'average_header.mif')

  else:
    progress = app.ProgressBar('Performing initial rigid registration to template', len(ins))
    mask_option = ''
    cid = 0
    lmax_option = ' -rigid_lmax 0 ' if cns.fod_reorientation[cid] else ' -noreorientation '
    contrast_weight_option = cns.initial_alignment_weight_option
    for inp in ins:
      output_option = ' -rigid ' + os.path.join('linear_transforms_initial', inp.uid + '.txt')
      images = ' '.join([p + ' ' + t for p, t in zip(inp.ims_path, cns.templates)])
      if use_masks:
        mask_option = ' -mask1 ' + inp.msk_path
        if initial_alignment == 'robust_mass':
          if not os.path.isfile('robust/template.mif'):
            if cns.n_volumes[cid] > 0:
              run.command('mrconvert ' + cns.templates[cid] + ' -coord 3 0 - | mrconvert - -axes 0,1,2 robust/template.mif')
            else:
              run.command('mrconvert ' + cns.templates[cid] + ' robust/template.mif')
          if n_contrasts > 1:
            cmd = ['mrcalc', inp.ims_path[cid], cns.mc_weight_initial_alignment[cid], '-mult']
            for cid in range(1, n_contrasts):
              cmd += [inp.ims_path[cid], cns.mc_weight_initial_alignment[cid], '-mult', '-add']
            contrast_weight_option = ''
            run.command(' '.join(cmd) +
                        ' - | mrfilter - zclean -zlower 3 -zupper 3 robust/image_' + inp.uid + '.mif'
                        ' -maskin ' + inp.msk_path + ' -maskout robust/mask_' + inp.uid + '.mif')
          else:
            run.command('mrfilter ' + inp.ims_path[0] + ' zclean -zlower 3 -zupper 3 robust/image_' + inp.uid + '.mif' +
                        ' -maskin ' + inp.msk_path + ' -maskout robust/mask_' + inp.uid + '.mif')
          images = 'robust/image_' + inp.uid + '.mif robust/template.mif'
          mask_option = ' -mask1 ' + 'robust/mask_' + inp.uid + '.mif'
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
      for cid in range(n_contrasts):
        run.command('mrtransform ' + inp.ims_path[cid] + c_mrtransform_reorientation[cid] +
                    ' -linear ' + os.path.join('linear_transforms_initial', inp.uid + '.txt') +
                    ' ' + inp.ims_transformed[cid] + "_translated.mif" + datatype_option)
      if use_masks:
        run.command('mrtransform ' + inp.msk_path +
                    ' -linear ' + os.path.join('linear_transforms_initial', inp.uid + '.txt') +
                    ' ' + inp.msk_transformed + "_translated.mif" +
                    datatype_option)
      progress.increment()
    # update average space of first contrast to new extent, delete other average space images
    run.command(['mraverageheader', [inp.ims_transformed[cid] + '_translated.mif' for inp in ins], 'average_header_tight.mif'])
    progress.done()

    if voxel_size is None:
      run.command('mrgrid average_header_tight.mif pad -uniform 10 average_header.mif', force=True)
    else:
      run.command('mrgrid average_header_tight.mif pad -uniform 10 - | '
                  'mrgrid - regrid -voxel ' + ','.join(map(str, voxel_size)) + ' average_header.mif', force=True)
    run.function(os.remove, 'average_header_tight.mif')
    for cid in range(1, n_contrasts):
      run.function(os.remove, 'average_header' + cns.suff[cid] + '.mif')

    if use_masks:
      # reslice masks
      progress = app.ProgressBar('Reslicing input masks to average header', len(ins))
      for inp in ins:
        run.command('mrtransform ' + inp.msk_transformed + '_translated.mif' + ' ' + inp.msk_transformed + ' ' +
                    '-interp nearest -template average_header.mif' + datatype_option)
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
        run.command('mrtransform ' + inp.msk_transformed + '_translated.mif ' + inp.msk_transformed + ' ' +
                    '-interp nearest -template average_header.mif' + datatype_option, force=True)
        run.function(os.remove, inp.msk_transformed + '_translated.mif')
        progress.increment()
      progress.done()
      run.function(os.remove, 'mask_translated.mif')

    # reslice images
    progress = app.ProgressBar('Reslicing input images to average header', len(ins) * n_contrasts)
    for cid in range(n_contrasts):
      for inp in ins:
        run.command('mrtransform ' + c_mrtransform_reorientation[cid] + inp.ims_transformed[cid] + '_translated.mif ' +
                    inp.ims_transformed[cid] + ' ' +
                    ' -interp linear -template average_header.mif' +
                    outofbounds_option +
                    datatype_option)
        run.function(os.remove, inp.ims_transformed[cid] + '_translated.mif')
        progress.increment()
    progress.done()

    if nanmask_input:
      inplace_nan_mask([inp.ims_transformed[cid] for inp in ins for cid in range(n_contrasts)],
                       [inp.msk_transformed for inp in ins for cid in range(n_contrasts)])

    if leave_one_out:
      calculate_isfinite(ins, cns)

  cns.templates = ['initial_template' + contrast + '.mif' for contrast in cns.suff]
  for cid in range(n_contrasts):
    aggregate(ins, 'initial_template' + cns.suff[cid] + '.mif', cid, agg_measure)
    if cns.n_volumes[cid] == 1:
      run.function(shutil.move, 'initial_template' + cns.suff[cid] + '.mif', 'tmp.mif')
      run.command('mrconvert tmp.mif initial_template' + cns.suff[cid] + '.mif -axes 0,1,2,-1')

  # Optimise template with linear registration
  if not dolinear:
    for inp in ins:
      run.function(copy, os.path.join('linear_transforms_initial', inp.uid+'.txt'),
                   os.path.join('linear_transforms', inp.uid+'.txt'))
  else:
    level = 0
    regtype = linear_type[0]
    def linear_msg():
      return 'Optimising template with linear registration (stage {0} of {1}; {2})'.format(level + 1, len(linear_scales), regtype)
    progress = app.ProgressBar(linear_msg, len(linear_scales) * len(ins) * (1 + n_contrasts + int(use_masks)))
    for level, (regtype, scale, niter, lmax) in enumerate(zip(linear_type, linear_scales, linear_niter, linear_lmax)):
      for inp in ins:
        initialise_option = ''
        if use_masks:
          mask_option = ' -mask1 ' + inp.msk_path
        else:
          mask_option = ''
        lmax_option = ' -noreorientation'
        metric_option = ''
        mrregister_log_option = ''
        if regtype == 'rigid':
          scale_option = ' -rigid_scale ' + str(scale)
          niter_option = ' -rigid_niter ' + str(niter)
          regtype_option = ' -type rigid'
          output_option = ' -rigid ' + os.path.join('linear_transforms_%02i' % level, inp.uid + '.txt')
          contrast_weight_option = cns.rigid_weight_option
          initialise_option = (' -rigid_init_matrix ' +
                               os.path.join('linear_transforms_%02i' % (level - 1) if level > 0 else 'linear_transforms_initial', inp.uid + '.txt'))
          if do_fod_registration:
            lmax_option = ' -rigid_lmax ' + str(lmax)
          if linear_estimator:
            metric_option = ' -rigid_metric.diff.estimator ' + linear_estimator
          if app.VERBOSITY >= 2:
            mrregister_log_option = ' -info -rigid_log ' + os.path.join('log', inp.uid + contrast[cid] + "_" + str(level) + '.log')
        else:
          scale_option = ' -affine_scale ' + str(scale)
          niter_option = ' -affine_niter ' + str(niter)
          regtype_option = ' -type affine'
          output_option = ' -affine ' + os.path.join('linear_transforms_%02i' % level, inp.uid + '.txt')
          contrast_weight_option = cns.affine_weight_option
          initialise_option = (' -affine_init_matrix ' +
                               os.path.join('linear_transforms_%02i' % (level - 1) if level > 0 else 'linear_transforms_initial', inp.uid + '.txt'))
          if do_fod_registration:
            lmax_option = ' -affine_lmax ' + str(lmax)
          if linear_estimator:
            metric_option = ' -affine_metric.diff.estimator ' + linear_estimator
          if write_log:
            mrregister_log_option = ' -info -affine_log ' + os.path.join('log', inp.uid + contrast[cid] + "_" + str(level) + '.log')

        if leave_one_out:
          tmpl = []
          for cid in range(n_contrasts):
            isfinite = 'isfinite%s/%s.mif' % (cns.suff[cid], inp.uid)
            weight = inp.aggregation_weight if inp.aggregation_weight is not None else '1'
            # loo = (template * weighted sum - weight * this) / (weighted sum - weight)
            run.command('mrcalc ' + cns.isfinite_count[cid] + ' ' + isfinite + ' -sub - | mrcalc ' + cns.templates[cid] +
                        ' ' + cns.isfinite_count[cid] + ' -mult ' + inp.ims_transformed[cid] + ' ' + weight + ' -mult ' +
                        ' -sub - -div loo_%s' % cns.templates[cid], force=True)
            tmpl.append('loo_%s' % cns.templates[cid])
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
        check_linear_transformation(os.path.join('linear_transforms_%02i' % level, inp.uid + '.txt'), command,
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
        run.command(['transformcalc', [os.path.join('linear_transforms_initial', inp.uid + '.txt') for _inp in ins],
                    'average', 'linear_transform_average_init.txt', '-quiet'], force=True)
        run.command(['transformcalc', [os.path.join('linear_transforms_%02i' % level, inp.uid + '.txt') for _inp in ins],
                    'average', 'linear_transform_average_%02i_uncorrected.txt' % level, '-quiet'], force=True)
        run.command(['transformcalc', 'linear_transform_average_%02i_uncorrected.txt' % level,
                    'invert', 'linear_transform_average_%02i_uncorrected_inv.txt' % level, '-quiet'], force=True)

        transform_average_init = matrix.load_transform('linear_transform_average_init.txt')
        transform_average_current_inv = matrix.load_transform('linear_transform_average_%02i_uncorrected_inv.txt' % level)

        transform_update = matrix.dot(transform_average_init, transform_average_current_inv)
        matrix.save_transform(os.path.join('linear_transforms_%02i_drift_correction.txt' %  level), transform_update, force=True)
        if regtype == 'rigid':
          run.command('transformcalc ' + os.path.join('linear_transforms_%02i_drift_correction.txt' %  level) +
                      ' rigid ' + os.path.join('linear_transforms_%02i_drift_correction.txt' %  level) + ' -quiet', force=True)
          transform_update = matrix.load_transform(os.path.join('linear_transforms_%02i_drift_correction.txt' %  level))

        for inp in ins:
          transform = matrix.load_transform('linear_transforms_%02i/' % level + inp.uid + '.txt')
          transform_updated = matrix.dot(transform, transform_update)
          run.function(copy, 'linear_transforms_%02i/' % level + inp.uid + '.txt', 'linear_transforms_%02i/' % level + inp.uid + '.precorrection')
          matrix.save_transform(os.path.join('linear_transforms_%02i' % level, inp.uid + '.txt'), transform_updated, force=True)

        # compute average trafos and its properties for easier debugging
        run.command(['transformcalc', [os.path.join('linear_transforms_%02i' % level, _inp.uid + '.txt') for _inp in ins],
                    'average', 'linear_transform_average_%02i.txt' % level, '-quiet'], force=True)
        run.command('transformcalc linear_transform_average_%02i.txt decompose linear_transform_average_%02i.dec' % (level, level), force=True)


      for cid in range(n_contrasts):
        for inp in ins:
          run.command('mrtransform ' + c_mrtransform_reorientation[cid] + inp.ims_path[cid] +
                      ' -template ' + cns.templates[cid] +
                      ' -linear ' + os.path.join('linear_transforms_%02i' % level, inp.uid + '.txt') +
                      ' ' + inp.ims_transformed[cid] +
                      outofbounds_option +
                      datatype_option,
                      force=True)
          progress.increment()

      if use_masks:
        for inp in ins:
          run.command('mrtransform ' + inp.msk_path +
                      ' -template ' + cns.templates[0] +
                      ' -interp nearest' +
                      ' -linear ' + os.path.join('linear_transforms_%02i' % level, inp.uid + '.txt') +
                      ' ' + inp.msk_transformed,
                      force=True)
          progress.increment()

      if nanmask_input:
        inplace_nan_mask([inp.ims_transformed[cid] for inp in ins for cid in range(n_contrasts)],
                         [inp.msk_transformed for inp in ins for cid in range(n_contrasts)])

      if leave_one_out:
        calculate_isfinite(ins, cns)

      for cid in range(n_contrasts):
        if level > 0 and app.ARGS.delete_temporary_files:
          os.remove(cns.templates[cid])
        cns.templates[cid] = 'linear_template%02i%s.mif' % (level, cns.suff[cid])
        aggregate(ins, cns.templates[cid], cid, agg_measure)
        if cns.n_volumes[cid] == 1:
          run.function(shutil.move, cns.templates[cid], 'tmp.mif')
          run.command('mrconvert tmp.mif ' + cns.templates[cid] + ' -axes 0,1,2,-1')
          run.function(os.remove, 'tmp.mif')

    for entry in os.listdir('linear_transforms_%02i' % level):
      run.function(copy, os.path.join('linear_transforms_%02i' % level, entry), os.path.join('linear_transforms', entry))
    progress.done()

  # Create a template mask for nl registration by taking the intersection of all transformed input masks and dilating
  if use_masks and (dononlinear or app.ARGS.template_mask):
    run.command(['mrmath', path.all_in_dir('mask_transformed')] +
                'min - | maskfilter - median - | maskfilter - dilate -npass 5 init_nl_template_mask.mif'.split(), force=True)
    current_template_mask = 'init_nl_template_mask.mif'

  if dononlinear:
    path.make_dir('warps')
    level = 0
    def nonlinear_msg():
      return 'Optimising template with non-linear registration (stage {0} of {1})'.format(level + 1, len(nl_scales))
    progress = app.ProgressBar(nonlinear_msg, len(nl_scales) * len(ins))
    for level, (scale, niter, lmax) in enumerate(zip(nl_scales, nl_niter, nl_lmax)):
      for inp in ins:
        if level > 0:
          initialise_option = ' -nl_init ' + os.path.join('warps_%02i' % (level - 1), inp.uid + '.mif')
          scale_option = ''
        else:
          scale_option = ' -nl_scale ' + str(scale)
          if not doaffine:  # rigid or no previous linear stage
            initialise_option = ' -rigid_init_matrix ' + os.path.join('linear_transforms', inp.uid + '.txt')
          else:
            initialise_option = ' -affine_init_matrix ' + os.path.join('linear_transforms', inp.uid + '.txt')

        if use_masks:
          mask_option = ' -mask1 ' + inp.msk_path + ' -mask2 ' + current_template_mask
        else:
          mask_option = ''

        if do_fod_registration:
          lmax_option = ' -nl_lmax ' + str(lmax)
        else:
          lmax_option = ' -noreorientation'

        contrast_weight_option = cns.nl_weight_option

        if leave_one_out:
          tmpl = []
          for cid in range(n_contrasts):
            isfinite = 'isfinite%s/%s.mif' % (cns.suff[cid], inp.uid)
            weight = inp.aggregation_weight if inp.aggregation_weight is not None else '1'
            # loo = (template * weighted sum - weight * this) / (weighted sum - weight)
            run.command('mrcalc ' + cns.isfinite_count[cid] + ' ' + isfinite + ' -sub - | mrcalc ' + cns.templates[cid] +
                        ' ' + cns.isfinite_count[cid] + ' -mult ' + inp.ims_transformed[cid] + ' ' + weight + ' -mult ' +
                        ' -sub - -div loo_%s' % cns.templates[cid], force=True)
            tmpl.append('loo_%s' % cns.templates[cid])
          images = ' '.join([p + ' ' + t for p, t in zip(inp.ims_path, tmpl)])
        else:
          images = ' '.join([p + ' ' + t for p, t in zip(inp.ims_path, cns.templates)])
        run.command('mrregister ' + images +
                    ' -type nonlinear' +
                    ' -nl_niter ' + str(nl_niter[level]) +
                    ' -nl_warp_full ' + os.path.join('warps_%02i' % level, inp.uid + '.mif') +
                    ' -transformed ' +
                    ' -transformed '.join([inp.ims_transformed[cid] for cid in range(n_contrasts)]) + ' ' +
                    ' -nl_update_smooth ' + app.ARGS.nl_update_smooth +
                    ' -nl_disp_smooth ' + app.ARGS.nl_disp_smooth +
                    ' -nl_grad_step ' + app.ARGS.nl_grad_step +
                    initialise_option +
                    contrast_weight_option +
                    scale_option +
                    mask_option +
                    datatype_option +
                    outofbounds_option +
                    lmax_option,
                    force=True)

        if use_masks:
          run.command('mrtransform ' + inp.msk_path +
                      ' -template ' + cns.templates[0] +
                      ' -warp_full ' + os.path.join('warps_%02i' % level, inp.uid + '.mif') +
                      ' ' + inp.msk_transformed +
                      ' -interp nearest ',
                      force=True)

        if leave_one_out:
          for im_temp in tmpl:
            run.function(os.remove, im_temp)

        if level > 0:
          run.function(os.remove, os.path.join('warps_%02i' % (level - 1), inp.uid + '.mif'))

        progress.increment(nonlinear_msg())

      if nanmask_input:
        inplace_nan_mask([_inp.ims_transformed[cid] for _inp in ins for cid in range(n_contrasts)],
                         [_inp.msk_transformed for _inp in ins for cid in range(n_contrasts)])

      if leave_one_out:
        calculate_isfinite(ins, cns)

      for cid in range(n_contrasts):
        if level > 0 and app.ARGS.delete_temporary_files:
          os.remove(cns.templates[cid])
        cns.templates[cid] = 'nl_template%02i%s.mif' % (level, cns.suff[cid])
        aggregate(ins, cns.templates[cid], cid, agg_measure)
        if cns.n_volumes[cid] == 1:
          run.function(shutil.move, cns.templates[cid], 'tmp.mif')
          run.command('mrconvert tmp.mif ' + cns.templates[cid] + ' -axes 0,1,2,-1')
          run.function(os.remove, 'tmp.mif')

      if use_masks:
        run.command(['mrmath', path.all_in_dir('mask_transformed')] +
                    'min - | maskfilter - median - | '.split() +
                    ('maskfilter - dilate -npass 5 nl_template_mask' + str(level) + '.mif').split())
        current_template_mask = 'nl_template_mask' + str(level) + '.mif'

      if level < len(nl_scales) - 1:
        if scale < nl_scales[level + 1]:
          upsample_factor = nl_scales[level + 1] / scale
          for inp in ins:
            run.command('mrgrid ' + os.path.join('warps_%02i' % level, inp.uid + '.mif') +
                        ' regrid -scale %f tmp.mif' % upsample_factor, force=True)
            run.function(shutil.move, 'tmp.mif', os.path.join('warps_%02i' % level, inp.uid + '.mif'))
      else:
        for inp in ins:
          run.function(shutil.move, os.path.join('warps_%02i' % level, inp.uid + '.mif'), 'warps')
    progress.done()

  for cid in range(n_contrasts):
    run.command('mrconvert ' + cns.templates[cid] + ' ' + cns.templates_out[cid],
                mrconvert_keyval='NULL', force=app.FORCE_OVERWRITE)

  if app.ARGS.warp_dir:
    warp_path = path.from_user(app.ARGS.warp_dir, False)
    if os.path.exists(warp_path):
      run.function(shutil.rmtree, warp_path)
    os.makedirs(warp_path)
    progress = app.ProgressBar('Copying non-linear warps to output directory "' + warp_path + '"', len(ins))
    for inp in ins:
      keyval = image.Header(os.path.join('warps', inp.uid + '.mif')).keyval()
      keyval = dict((k, keyval[k]) for k in ('linear1', 'linear2'))
      json_path = os.path.join('warps', inp.uid + '.json')
      with open(json_path, 'w', encoding='utf-8') as json_file:
        json.dump(keyval, json_file)
      run.command('mrconvert ' + os.path.join('warps', inp.uid + '.mif') + ' ' +
                  shlex.quote(os.path.join(warp_path, xcontrast_xsubject_pre_postfix[0] +
                                           inp.uid + xcontrast_xsubject_pre_postfix[1] + '.mif')),
                  mrconvert_keyval=json_path, force=app.FORCE_OVERWRITE)
      progress.increment()
    progress.done()

  if app.ARGS.linear_transformations_dir:
    linear_transformations_path = path.from_user(app.ARGS.linear_transformations_dir, False)
    if os.path.exists(linear_transformations_path):
      run.function(shutil.rmtree, linear_transformations_path)
    os.makedirs(linear_transformations_path)
    for inp in ins:
      trafo = matrix.load_transform(os.path.join('linear_transforms', inp.uid + '.txt'))
      matrix.save_transform(os.path.join(linear_transformations_path,
                                         xcontrast_xsubject_pre_postfix[0] + inp.uid
                                         + xcontrast_xsubject_pre_postfix[1] + '.txt'),
                            trafo,
                            force=app.FORCE_OVERWRITE)

  if app.ARGS.transformed_dir:
    for cid, trdir in enumerate(app.ARGS.transformed_dir):
      transformed_path = path.from_user(trdir, False)
      if os.path.exists(transformed_path):
        run.function(shutil.rmtree, transformed_path)
      os.makedirs(transformed_path)
      progress = app.ProgressBar('Copying transformed images to output directory "' + transformed_path + '"', len(ins))
      for inp in ins:
        run.command(['mrconvert', inp.ims_transformed[cid], os.path.join(transformed_path, inp.ims_filenames[cid])],
                    mrconvert_keyval=inp.get_ims_path(False)[cid], force=app.FORCE_OVERWRITE)
        progress.increment()
      progress.done()

  if app.ARGS.template_mask:
    run.command('mrconvert ' + current_template_mask + ' ' + path.from_user(app.ARGS.template_mask, True),
                mrconvert_keyval='NULL', force=app.FORCE_OVERWRITE)
