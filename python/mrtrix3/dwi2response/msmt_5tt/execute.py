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

import os, shlex, shutil
from mrtrix3 import MRtrixError
from mrtrix3 import app, image, path, run

def execute(): #pylint: disable=unused-variable
  # Ideally want to use the oversampling-based regridding of the 5TT image from the SIFT model, not mrtransform
  # May need to commit 5ttregrid...

  # Verify input 5tt image
  verification_text = ''
  try:
    verification_text = run.command('5ttcheck 5tt.mif').stderr
  except run.MRtrixCmdError as except_5ttcheck:
    verification_text = except_5ttcheck.stderr
  if 'WARNING' in verification_text or 'ERROR' in verification_text:
    app.warn('Command 5ttcheck indicates problems with provided input 5TT image \'' + app.ARGS.in_5tt + '\':')
    for line in verification_text.splitlines():
      app.warn(line)
    app.warn('These may or may not interfere with the dwi2response msmt_5tt script')

  # Get shell information
  shells = [ int(round(float(x))) for x in image.mrinfo('dwi.mif', 'shell_bvalues').split() ]
  if len(shells) < 3:
    app.warn('Less than three b-values; response functions will not be applicable in resolving three tissues using MSMT-CSD algorithm')

  # Get lmax information (if provided)
  wm_lmax = [ ]
  if app.ARGS.lmax:
    wm_lmax = [ int(x.strip()) for x in app.ARGS.lmax.split(',') ]
    if not len(wm_lmax) == len(shells):
      raise MRtrixError('Number of manually-defined lmax\'s (' + str(len(wm_lmax)) + ') does not match number of b-values (' + str(len(shells)) + ')')
    for shell_l in wm_lmax:
      if shell_l % 2:
        raise MRtrixError('Values for lmax must be even')
      if shell_l < 0:
        raise MRtrixError('Values for lmax must be non-negative')

  run.command('dwi2tensor dwi.mif - -mask mask.mif | tensor2metric - -fa fa.mif -vector vector.mif')
  if not os.path.exists('dirs.mif'):
    run.function(shutil.copy, 'vector.mif', 'dirs.mif')
  run.command('mrtransform 5tt.mif 5tt_regrid.mif -template fa.mif -interp linear')

  # Basic tissue masks
  run.command('mrconvert 5tt_regrid.mif - -coord 3 2 -axes 0,1,2 | mrcalc - ' + str(app.ARGS.pvf) + ' -gt mask.mif -mult wm_mask.mif')
  run.command('mrconvert 5tt_regrid.mif - -coord 3 0 -axes 0,1,2 | mrcalc - ' + str(app.ARGS.pvf) + ' -gt fa.mif ' + str(app.ARGS.fa) + ' -lt -mult mask.mif -mult gm_mask.mif')
  run.command('mrconvert 5tt_regrid.mif - -coord 3 3 -axes 0,1,2 | mrcalc - ' + str(app.ARGS.pvf) + ' -gt fa.mif ' + str(app.ARGS.fa) + ' -lt -mult mask.mif -mult csf_mask.mif')

  # Revise WM mask to only include single-fibre voxels
  recursive_cleanup_option=''
  if not app.DO_CLEANUP:
    recursive_cleanup_option = ' -nocleanup'
  if not app.ARGS.sfwm_fa_threshold:
    app.console('Selecting WM single-fibre voxels using \'' + app.ARGS.wm_algo + '\' algorithm')
    run.command('dwi2response ' + app.ARGS.wm_algo + ' dwi.mif wm_ss_response.txt -mask wm_mask.mif -voxels wm_sf_mask.mif -scratch ' + shlex.quote(app.SCRATCH_DIR) + recursive_cleanup_option)
  else:
    app.console('Selecting WM single-fibre voxels using \'fa\' algorithm with a hard FA threshold of ' + str(app.ARGS.sfwm_fa_threshold))
    run.command('dwi2response fa dwi.mif wm_ss_response.txt -mask wm_mask.mif -threshold ' + str(app.ARGS.sfwm_fa_threshold) + ' -voxels wm_sf_mask.mif -scratch ' + shlex.quote(app.SCRATCH_DIR) + recursive_cleanup_option)

  # Check for empty masks
  wm_voxels  = image.statistics('wm_sf_mask.mif', mask='wm_sf_mask.mif').count
  gm_voxels  = image.statistics('gm_mask.mif',    mask='gm_mask.mif').count
  csf_voxels = image.statistics('csf_mask.mif',   mask='csf_mask.mif').count
  empty_masks = [ ]
  if not wm_voxels:
    empty_masks.append('WM')
  if not gm_voxels:
    empty_masks.append('GM')
  if not csf_voxels:
    empty_masks.append('CSF')
  if empty_masks:
    message = ','.join(empty_masks)
    message += ' tissue mask'
    if len(empty_masks) > 1:
      message += 's'
    message += ' empty; cannot estimate response function'
    if len(empty_masks) > 1:
      message += 's'
    raise MRtrixError(message)

  # For each of the three tissues, generate a multi-shell response
  bvalues_option = ' -shells ' + ','.join(map(str,shells))
  sfwm_lmax_option = ''
  if wm_lmax:
    sfwm_lmax_option = ' -lmax ' + ','.join(map(str,wm_lmax))
  run.command('amp2response dwi.mif wm_sf_mask.mif dirs.mif wm.txt' + bvalues_option + sfwm_lmax_option)
  run.command('amp2response dwi.mif gm_mask.mif dirs.mif gm.txt' + bvalues_option + ' -isotropic')
  run.command('amp2response dwi.mif csf_mask.mif dirs.mif csf.txt' + bvalues_option + ' -isotropic')
  run.function(shutil.copyfile, 'wm.txt',  path.from_user(app.ARGS.out_wm,  False))
  run.function(shutil.copyfile, 'gm.txt',  path.from_user(app.ARGS.out_gm,  False))
  run.function(shutil.copyfile, 'csf.txt', path.from_user(app.ARGS.out_csf, False))

  # Generate output 4D binary image with voxel selections; RGB as in MSMT-CSD paper
  run.command('mrcat csf_mask.mif gm_mask.mif wm_sf_mask.mif voxels.mif -axis 3')
  if app.ARGS.voxels:
    run.command('mrconvert voxels.mif ' + path.from_user(app.ARGS.voxels), mrconvert_keyval=path.from_user(app.ARGS.input, False), force=app.FORCE_OVERWRITE)
