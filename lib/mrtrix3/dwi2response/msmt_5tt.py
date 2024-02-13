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
from mrtrix3 import app, image, run


NEEDS_SINGLE_SHELL = False # pylint: disable=unused-variable
SUPPORTS_MASK = True # pylint: disable=unused-variable

WM_ALGOS = [ 'fa', 'tax', 'tournier' ]



def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('msmt_5tt', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Derive MSMT-CSD tissue response functions based on a co-registered five-tissue-type (5TT) image')
  parser.add_citation('Jeurissen, B.; Tournier, J.-D.; Dhollander, T.; Connelly, A. & Sijbers, J. '
                      'Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data. '
                      'NeuroImage, 2014, 103, 411-426')
  parser.add_argument('input',
                      type=app.Parser.ImageIn(),
                      help='The input DWI')
  parser.add_argument('in_5tt',
                      type=app.Parser.ImageIn(),
                      help='Input co-registered 5TT image')
  parser.add_argument('out_wm',
                      type=app.Parser.FileOut(),
                      help='Output WM response text file')
  parser.add_argument('out_gm',
                      type=app.Parser.FileOut(),
                      help='Output GM response text file')
  parser.add_argument('out_csf',
                      type=app.Parser.FileOut(),
                      help='Output CSF response text file')
  options = parser.add_argument_group('Options specific to the "msmt_5tt" algorithm')
  options.add_argument('-dirs',
                       type=app.Parser.ImageIn(),
                       metavar='image',
                       help='Provide an input image that contains a pre-estimated fibre direction in each voxel '
                            '(a tensor fit will be used otherwise)')
  options.add_argument('-fa',
                       type=app.Parser.Float(0.0, 1.0),
                       metavar='value',
                       default=0.2,
                       help='Upper fractional anisotropy threshold for GM and CSF voxel selection '
                            '(default: 0.2)')
  options.add_argument('-pvf',
                       type=app.Parser.Float(0.0, 1.0),
                       metavar='fraction',
                       default=0.95,
                       help='Partial volume fraction threshold for tissue voxel selection '
                            '(default: 0.95)')
  options.add_argument('-wm_algo',
                       metavar='algorithm',
                       choices=WM_ALGOS,
                       default='tournier',
                       help='dwi2response algorithm to use for WM single-fibre voxel selection '
                            f'(options: {", ".join(WM_ALGOS)}; default: tournier)')
  options.add_argument('-sfwm_fa_threshold',
                       type=app.Parser.Float(0.0, 1.0),
                       metavar='value',
                       help='Sets -wm_algo to fa and allows to specify a hard FA threshold for single-fibre WM voxels, '
                            'which is passed to the -threshold option of the fa algorithm '
                            '(warning: overrides -wm_algo option)')



def execute(): #pylint: disable=unused-variable
  # Ideally want to use the oversampling-based regridding of the 5TT image from the SIFT model, not mrtransform
  # May need to commit 5ttregrid...

  run.command(['mrconvert', app.ARGS.in_5tt, '5tt.mif'],
              preserve_pipes=True)
  if app.ARGS.dirs:
    run.command(['mrconvert', app.ARGS.dirs, 'dirs.mif', '-strides', '0,0,0,1'],
                preserve_pipes=True)

  # Verify input 5tt image
  verification_text = ''
  try:
    verification_text = run.command('5ttcheck 5tt.mif').stderr
  except run.MRtrixCmdError as except_5ttcheck:
    verification_text = except_5ttcheck.stderr
  if 'WARNING' in verification_text or 'ERROR' in verification_text:
    app.warn(f'Command 5ttcheck indicates problems with provided input 5TT image "{app.ARGS.in_5tt}":')
    for line in verification_text.splitlines():
      app.warn(line)
    app.warn('These may or may not interfere with the dwi2response msmt_5tt script')

  # Get shell information
  shells = [ int(round(float(x))) for x in image.mrinfo('dwi.mif', 'shell_bvalues').split() ]
  if len(shells) < 3:
    app.warn('Less than three b-values; '
             'response functions will not be applicable in resolving three tissues using MSMT-CSD algorithm')

  # Get lmax information (if provided)
  sfwm_lmax_option = ''
  if app.ARGS.lmax:
    if len(app.ARGS.lmax) != len(shells):
      raise MRtrixError(f'Number of manually-defined lmax\'s ({len(app.ARGS.lmax)}) '
                        f'does not match number of b-values ({len(shells)})')
    if any(l % 2 for l in app.ARGS.lmax):
      raise MRtrixError('Values for lmax must be even')
    if any(l < 0 for l in app.ARGS.lmax):
      raise MRtrixError('Values for lmax must be non-negative')
    sfwm_lmax_option = ' -lmax ' + ','.join(map(str,app.ARGS.lmax))

  run.command('dwi2tensor dwi.mif - -mask mask.mif | '
              'tensor2metric - -fa fa.mif -vector vector.mif')
  if not os.path.exists('dirs.mif'):
    run.function(shutil.copy, 'vector.mif', 'dirs.mif')
  run.command('mrtransform 5tt.mif 5tt_regrid.mif -template fa.mif -interp linear')

  # Basic tissue masks
  run.command('mrconvert 5tt_regrid.mif - -coord 3 2 -axes 0,1,2 | '
              f'mrcalc - {app.ARGS.pvf} -gt mask.mif -mult wm_mask.mif')
  run.command('mrconvert 5tt_regrid.mif - -coord 3 0 -axes 0,1,2 | '
              f'mrcalc - {app.ARGS.pvf} -gt fa.mif {app.ARGS.fa} -lt -mult mask.mif -mult gm_mask.mif')
  run.command('mrconvert 5tt_regrid.mif - -coord 3 3 -axes 0,1,2 | '
              f'mrcalc - {app.ARGS.pvf} -gt fa.mif {app.ARGS.fa} -lt -mult mask.mif -mult csf_mask.mif')

  # Revise WM mask to only include single-fibre voxels
  recursive_cleanup_option=''
  if not app.DO_CLEANUP:
    recursive_cleanup_option = ' -nocleanup'
  if not app.ARGS.sfwm_fa_threshold:
    app.console(f'Selecting WM single-fibre voxels using "{app.ARGS.wm_algo}" algorithm')
    run.command(f'dwi2response {app.ARGS.wm_algo} dwi.mif wm_ss_response.txt -mask wm_mask.mif -voxels wm_sf_mask.mif'
                ' -scratch %s' % shlex.quote(app.SCRATCH_DIR)
                + recursive_cleanup_option)
  else:
    app.console(f'Selecting WM single-fibre voxels using "fa" algorithm with a hard FA threshold of {app.ARGS.sfwm_fa_threshold}')
    run.command(f'dwi2response fa dwi.mif wm_ss_response.txt -mask wm_mask.mif -threshold {app.ARGS.sfwm_fa_threshold} -voxels wm_sf_mask.mif'
                ' -scratch %s' % shlex.quote(app.SCRATCH_DIR)
                + recursive_cleanup_option)

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
    raise MRtrixError(f'{",".join(empty_masks)} tissue {("masks" if len(empty_masks) > 1 else "mask")} empty; '
                      f'cannot estimate response {"functions" if len(empty_masks) > 1 else "function"}')

  # For each of the three tissues, generate a multi-shell response
  bvalues_option = f' -shells {",".join(map(str,shells))}'
  run.command(f'amp2response dwi.mif wm_sf_mask.mif dirs.mif wm.txt {bvalues_option} {sfwm_lmax_option}')
  run.command(f'amp2response dwi.mif gm_mask.mif dirs.mif gm.txt {bvalues_option} -isotropic')
  run.command(f'amp2response dwi.mif csf_mask.mif dirs.mif csf.txt {bvalues_option} -isotropic')
  run.function(shutil.copyfile, 'wm.txt',  app.ARGS.out_wm)
  run.function(shutil.copyfile, 'gm.txt',  app.ARGS.out_gm)
  run.function(shutil.copyfile, 'csf.txt', app.ARGS.out_csf)

  # Generate output 4D binary image with voxel selections; RGB as in MSMT-CSD paper
  run.command('mrcat csf_mask.mif gm_mask.mif wm_sf_mask.mif voxels.mif -axis 3')
  if app.ARGS.voxels:
    run.command(['mrconvert', 'voxels.mif', app.ARGS.voxels],
                mrconvert_keyval=app.ARGS.input,
                force=app.FORCE_OVERWRITE,
                preserve_pipes=True)
