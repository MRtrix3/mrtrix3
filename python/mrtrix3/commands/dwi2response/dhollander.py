# Copyright (c) 2008-2025 the MRtrix3 contributors.
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


import math, shlex, shutil
from mrtrix3 import BZERO_THRESHOLD_DEFAULT, CONFIG, MRtrixError
from mrtrix3 import app, image, run


NEEDS_SINGLE_SHELL = False # pylint: disable=unused-variable
SUPPORTS_MASK = True # pylint: disable=unused-variable

WM_ALGOS = [ 'fa', 'tax', 'tournier' ]



def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('dhollander', parents=[base_parser])
  parser.set_author('Thijs Dhollander (thijs.dhollander@gmail.com)')
  parser.set_synopsis('Unsupervised estimation of WM, GM and CSF response functions '
                      'that does not require a T1 image (or segmentation thereof)')
  parser.add_description('This is an improved version of the Dhollander et al. (2016) algorithm'
                         ' for unsupervised estimation of WM, GM and CSF response functions,'
                         ' which includes the Dhollander et al. (2019) improvements'
                         ' for single-fibre WM response function estimation'
                         ' (prior to this update,'
                         ' the "dwi2response tournier" algorithm had been utilised'
                         ' specifically for the single-fibre WM response function estimation step).')
  parser.add_citation('Dhollander, T.; Raffelt, D. & Connelly, A. '
                      'Unsupervised 3-tissue response function estimation from single-shell or multi-shell diffusion MR data without a co-registered T1 image. '
                      'ISMRM Workshop on Breaking the Barriers of Diffusion MRI, 2016, 5')
  parser.add_citation('Dhollander, T.; Mito, R.; Raffelt, D. & Connelly, A. '
                      'Improved white matter response function estimation for 3-tissue constrained spherical deconvolution. '
                      'Proc Intl Soc Mag Reson Med, 2019, 555',
                      condition='If -wm_algo option is not used')
  parser.add_argument('input',
                      type=app.Parser.ImageIn(),
                      help='Input DWI dataset')
  parser.add_argument('out_sfwm',
                      type=app.Parser.FileOut(),
                      help='Output single-fibre WM response function text file')
  parser.add_argument('out_gm',
                      type=app.Parser.FileOut(),
                      help='Output GM response function text file')
  parser.add_argument('out_csf',
                      type=app.Parser.FileOut(),
                      help='Output CSF response function text file')
  options = parser.add_argument_group('Options for the "dhollander" algorithm')
  options.add_argument('-erode',
                       type=app.Parser.Int(0),
                       metavar='iterations',
                       default=3,
                       help='Number of erosion passes to apply to initial (whole brain) mask. '
                            'Set to 0 to not erode the brain mask. '
                            '(default: 3)')
  options.add_argument('-fa',
                       type=app.Parser.Float(0.0, 1.0),
                       metavar='threshold',
                       default=0.2,
                       help='FA threshold for crude WM versus GM-CSF separation. '
                            '(default: 0.2)')
  options.add_argument('-sfwm',
                       type=app.Parser.Float(0.0, 100.0),
                       metavar='percentage',
                       default=0.5,
                       help='Final number of single-fibre WM voxels to select, '
                            'as a percentage of refined WM. '
                            '(default: 0.5 per cent)')
  options.add_argument('-gm',
                       type=app.Parser.Float(0.0, 100.0),
                       metavar='percentage',
                       default=2.0,
                       help='Final number of GM voxels to select, '
                            'as a percentage of refined GM. '
                            '(default: 2 per cent)')
  options.add_argument('-csf',
                       type=app.Parser.Float(0.0, 100.0),
                       metavar='percentage',
                       default=10.0,
                       help='Final number of CSF voxels to select, '
                            'as a percentage of refined CSF. '
                            '(default: 10 per cent)')
  options.add_argument('-wm_algo',
                       metavar='algorithm',
                       choices=WM_ALGOS,
                       help='Use external dwi2response algorithm for WM single-fibre voxel selection '
                            f'(options: {", ".join(WM_ALGOS)}) '
                            '(default: built-in Dhollander 2019)')



def execute(): #pylint: disable=unused-variable
  bzero_threshold = float(CONFIG['BZeroThreshold']) if 'BZeroThreshold' in CONFIG else BZERO_THRESHOLD_DEFAULT


  # CHECK INPUTS AND OPTIONS
  app.console('-------')

  # Get b-values and number of volumes per b-value.
  bvalues = [ int(round(float(x))) for x in image.mrinfo('dwi.mif', 'shell_bvalues').split() ]
  bvolumes = [ int(x) for x in image.mrinfo('dwi.mif', 'shell_sizes').split() ]
  app.console(f'{len(bvalues)} unique b-value(s) detected: '
              f'{",".join(map(str,bvalues))} with {",".join(map(str,bvolumes))} volumes')
  if len(bvalues) < 2:
    raise MRtrixError('Need at least 2 unique b-values (including b=0).')
  bvalues_option = f' -shells {",".join(map(str,bvalues))}'

  # Get lmax information (if provided).
  sfwm_lmax_option = ''
  sfwm_lmax = [ ]
  if app.ARGS.lmax:
    sfwm_lmax = app.ARGS.lmax
    if len(sfwm_lmax) != len(bvalues):
      raise MRtrixError(f'Number of lmax\'s ({len(sfwm_lmax)}, as supplied to the -lmax option: '
                        f'{",".join(map(str,sfwm_lmax))}) does not match number of unique b-values.')
    if any(sfl%2 for sfl in sfwm_lmax):
      raise MRtrixError('Values supplied to the -lmax option must be even.')
    if any(sfl<0 for sfl in sfwm_lmax):
      raise MRtrixError('Values supplied to the -lmax option must be non-negative.')
    sfwm_lmax_option = ' -lmax ' + ','.join(map(str,sfwm_lmax))


  # PREPARATION
  app.console('-------')
  app.console('Preparation:')

  # Erode (brain) mask.
  if app.ARGS.erode > 0:
    app.console('* Eroding brain mask by ' + str(app.ARGS.erode) + ' pass(es)...')
    run.command(f'maskfilter mask.mif erode eroded_mask.mif -npass {app.ARGS.erode}', show=False)
  else:
    app.console('Not eroding brain mask.')
    run.command('mrconvert mask.mif eroded_mask.mif -datatype bit', show=False)
  statmaskcount = image.statistics('mask.mif', mask='mask.mif').count
  statemaskcount = image.statistics('eroded_mask.mif', mask='eroded_mask.mif').count
  app.console(f'  [ mask: {statmaskcount} -> {statemaskcount} ]')

  # Get volumes, compute mean signal and SDM per b-value; compute overall SDM; get rid of erroneous values.
  app.console('* Computing signal decay metric (SDM):')
  totvolumes = 0
  fullsdmcmd = 'mrcalc'
  errcmd = 'mrcalc'
  zeropath = f'mean_b{bvalues[0]}.mif'
  for ibv, bval in enumerate(bvalues):
    app.console(f' * b={bval}...')
    meanpath = f'mean_b{bval}.mif'
    run.command(f'dwiextract dwi.mif -shells {bval} - | '
                f'mrcalc - 0 -max - | '
                f'mrmath - mean {meanpath} -axis 3',
                show=False)
    errpath = f'err_b{bval}.mif'
    run.command(f'mrcalc {meanpath} -finite {meanpath} 0 -if 0 -le {errpath} -datatype bit', show=False)
    errcmd += f' {errpath}'
    if ibv>0:
      errcmd += ' -add'
      sdmpath = f'sdm_b{bval}.mif'
      run.command(f'mrcalc {zeropath} {meanpath} -divide -log {sdmpath}', show=False)
      totvolumes += bvolumes[ibv]
      fullsdmcmd += f' {sdmpath} {bvolumes[ibv]} -mult'
      if ibv>1:
        fullsdmcmd += ' -add'
  fullsdmcmd += f' {totvolumes} -divide full_sdm.mif'
  run.command(fullsdmcmd, show=False)
  app.console('* Removing erroneous voxels from mask and correcting SDM...')
  run.command('mrcalc full_sdm.mif -finite full_sdm.mif 0 -if 0 -le err_sdm.mif -datatype bit', show=False)
  errcmd += ' err_sdm.mif -add 0 eroded_mask.mif -if safe_mask.mif -datatype bit'
  run.command(errcmd, show=False)
  run.command('mrcalc safe_mask.mif full_sdm.mif 0 -if 10 -min safe_sdm.mif', show=False)
  statsmaskcount = image.statistics('safe_mask.mif', mask='safe_mask.mif').count
  app.console(f'  [ mask: {statemaskcount} -> {statsmaskcount} ]')


  # CRUDE SEGMENTATION
  app.console('-------')
  app.console('Crude segmentation:')

  # Compute FA and principal eigenvectors; crude WM versus GM-CSF separation based on FA.
  app.console('* Crude WM versus GM-CSF separation (at FA=' + str(app.ARGS.fa) + ')...')
  run.command('dwi2tensor dwi.mif - -mask safe_mask.mif | '
              'tensor2metric - -fa safe_fa.mif -vector safe_vecs.mif -modulate none -mask safe_mask.mif',
              show=False)
  run.command(f'mrcalc safe_mask.mif safe_fa.mif 0 -if {app.ARGS.fa} -gt crude_wm.mif -datatype bit', show=False)
  run.command('mrcalc crude_wm.mif 0 safe_mask.mif -if _crudenonwm.mif -datatype bit', show=False)
  statcrudewmcount = image.statistics('crude_wm.mif', mask='crude_wm.mif').count
  statcrudenonwmcount = image.statistics('_crudenonwm.mif', mask='_crudenonwm.mif').count
  app.console(f'  [ {statsmaskcount} -> {statcrudewmcount} (WM) & {statcrudenonwmcount} (GM-CSF) ]')

  # Crude GM versus CSF separation based on SDM.
  app.console('* Crude GM versus CSF separation...')
  crudenonwmmedian = image.statistics('safe_sdm.mif', mask='_crudenonwm.mif').median
  run.command(f'mrcalc _crudenonwm.mif safe_sdm.mif {crudenonwmmedian} -subtract 0 -if - | '
              'mrthreshold - - -mask _crudenonwm.mif | '
              'mrcalc _crudenonwm.mif - 0 -if crude_csf.mif -datatype bit',
              show=False)
  run.command('mrcalc crude_csf.mif 0 _crudenonwm.mif -if crude_gm.mif -datatype bit', show=False)
  statcrudegmcount = image.statistics('crude_gm.mif', mask='crude_gm.mif').count
  statcrudecsfcount = image.statistics('crude_csf.mif', mask='crude_csf.mif').count
  app.console(f'  [ {statcrudenonwmcount} -> {statcrudegmcount} (GM) & {statcrudecsfcount} (CSF) ]')


  # REFINED SEGMENTATION
  app.console('-------')
  app.console('Refined segmentation:')

  # Refine WM: remove high SDM outliers.
  app.console('* Refining WM...')
  crudewmmedian = image.statistics('safe_sdm.mif', mask='crude_wm.mif').median
  run.command(f'mrcalc crude_wm.mif safe_sdm.mif {crudewmmedian} -subtract -abs 0 -if _crudewm_sdmad.mif', show=False)
  crudewmmad = image.statistics('_crudewm_sdmad.mif', mask='crude_wm.mif').median
  crudewmoutlthresh = crudewmmedian + (1.4826 * crudewmmad * 2.0)
  run.command(f'mrcalc crude_wm.mif safe_sdm.mif 0 -if {crudewmoutlthresh} -gt _crudewmoutliers.mif -datatype bit', show=False)
  run.command('mrcalc _crudewmoutliers.mif 0 crude_wm.mif -if refined_wm.mif -datatype bit', show=False)
  statrefwmcount = image.statistics('refined_wm.mif', mask='refined_wm.mif').count
  app.console(f'  [ WM: {statcrudewmcount} -> {statrefwmcount} ]')

  # Refine GM: separate safer GM from partial volumed voxels.
  app.console('* Refining GM...')
  crudegmmedian = image.statistics('safe_sdm.mif', mask='crude_gm.mif').median
  run.command(f'mrcalc crude_gm.mif safe_sdm.mif 0 -if {crudegmmedian} -gt _crudegmhigh.mif -datatype bit', show=False)
  run.command('mrcalc _crudegmhigh.mif 0 crude_gm.mif -if _crudegmlow.mif -datatype bit', show=False)
  run.command(f'mrcalc _crudegmhigh.mif safe_sdm.mif {crudegmmedian} -subtract 0 -if - | '
              'mrthreshold - - -mask _crudegmhigh.mif -invert | '
              'mrcalc _crudegmhigh.mif - 0 -if _crudegmhighselect.mif -datatype bit',
              show=False)
  run.command(f'mrcalc _crudegmlow.mif safe_sdm.mif {crudegmmedian} -subtract -neg 0 -if - | '
              'mrthreshold - - -mask _crudegmlow.mif -invert | '
              'mrcalc _crudegmlow.mif - 0 -if _crudegmlowselect.mif -datatype bit', show=False)
  run.command('mrcalc _crudegmhighselect.mif 1 _crudegmlowselect.mif -if refined_gm.mif -datatype bit', show=False)
  statrefgmcount = image.statistics('refined_gm.mif', mask='refined_gm.mif').count
  app.console(f'  [ GM: {statcrudegmcount} -> {statrefgmcount} ]')

  # Refine CSF: recover lost CSF from crude WM SDM outliers, separate safer CSF from partial volumed voxels.
  app.console('* Refining CSF...')
  crudecsfmin = image.statistics('safe_sdm.mif', mask='crude_csf.mif').min
  run.command(f'mrcalc _crudewmoutliers.mif safe_sdm.mif 0 -if {crudecsfmin} -gt 1 crude_csf.mif -if _crudecsfextra.mif -datatype bit', show=False)
  run.command(f'mrcalc _crudecsfextra.mif safe_sdm.mif {crudecsfmin} -subtract 0 -if - | '
              'mrthreshold - - -mask _crudecsfextra.mif | '
              'mrcalc _crudecsfextra.mif - 0 -if refined_csf.mif -datatype bit',
              show=False)
  statrefcsfcount = image.statistics('refined_csf.mif', mask='refined_csf.mif').count
  app.console(f'  [ CSF: {statcrudecsfcount} -> {statrefcsfcount} ]')


  # FINAL VOXEL SELECTION AND RESPONSE FUNCTION ESTIMATION
  app.console('-------')
  app.console('Final voxel selection and response function estimation:')

  # Get final voxels for CSF response function estimation from refined CSF.
  app.console('* CSF:')
  app.console(f' * Selecting final voxels ({app.ARGS.csf}% of refined CSF)...')
  voxcsfcount = int(round(statrefcsfcount * app.ARGS.csf / 100.0))
  run.command('mrcalc refined_csf.mif safe_sdm.mif 0 -if - | '
              f'mrthreshold - - -top {voxcsfcount} -ignorezero | '
              'mrcalc refined_csf.mif - 0 -if - -datatype bit | '
              'mrconvert - voxels_csf.mif -axes 0,1,2',
              show=False)
  statvoxcsfcount = image.statistics('voxels_csf.mif', mask='voxels_csf.mif').count
  app.console(f'   [ CSF: {statrefcsfcount} -> {statvoxcsfcount} ]')
  # Estimate CSF response function
  app.console(' * Estimating response function...')
  run.command(f'amp2response dwi.mif voxels_csf.mif safe_vecs.mif response_csf.txt {bvalues_option} -isotropic', show=False)

  # Get final voxels for GM response function estimation from refined GM.
  app.console('* GM:')
  app.console(f' * Selecting final voxels ({app.ARGS.gm}% of refined GM)...')
  voxgmcount = int(round(statrefgmcount * app.ARGS.gm / 100.0))
  refgmmedian = image.statistics('safe_sdm.mif', mask='refined_gm.mif').median
  run.command(f'mrcalc refined_gm.mif safe_sdm.mif {refgmmedian} -subtract -abs 1 -add 0 -if - | '
              f'mrthreshold - - -bottom {voxgmcount} -ignorezero | '
              'mrcalc refined_gm.mif - 0 -if - -datatype bit | '
              'mrconvert - voxels_gm.mif -axes 0,1,2',
              show=False)
  statvoxgmcount = image.statistics('voxels_gm.mif', mask='voxels_gm.mif').count
  app.console(f'   [ GM: {statrefgmcount} -> {statvoxgmcount} ]')
  # Estimate GM response function
  app.console(' * Estimating response function...')
  run.command(f'amp2response dwi.mif voxels_gm.mif safe_vecs.mif response_gm.txt {bvalues_option} -isotropic', show=False)

  # Get final voxels for single-fibre WM response function estimation from refined WM.
  app.console('* Single-fibre WM:')
  sfwm_message = '' if app.ARGS.wm_algo == "tax" else f' ({app.ARGS.sfwm}% of refined WM)'
  app.console(f' * Selecting final voxels{sfwm_message}...')
  voxsfwmcount = int(round(statrefwmcount * app.ARGS.sfwm / 100.0))

  if app.ARGS.wm_algo:
    recursive_cleanup_option=''
    if not app.DO_CLEANUP:
      recursive_cleanup_option = ' -nocleanup'
    app.console(f'   Selecting WM single-fibre voxels using "{app.ARGS.wm_algo}" algorithm')
    if app.ARGS.wm_algo == 'tax' and app.ARGS.sfwm != 0.5:
      app.warn('Single-fibre WM response function selection algorithm "tax" will not honour requested WM voxel percentage')
    run.command(f'dwi2response {app.ARGS.wm_algo} dwi.mif _respsfwmss.txt -mask refined_wm.mif -voxels voxels_sfwm.mif'
                + ('' if app.ARGS.wm_algo == 'tax' else (' -number ' + str(voxsfwmcount)))
                + ' -scratch ' + shlex.quote(app.SCRATCH_DIR)
                + recursive_cleanup_option,
                show=False)
  else:
    app.console('   Selecting WM single-fibre voxels using built-in (Dhollander et al., 2019) algorithm')
    run.command('mrmath dwi.mif mean mean_sig.mif -axis 3', show=False)
    refwmcoef = image.statistics('mean_sig.mif', mask='refined_wm.mif').median * math.sqrt(4.0 * math.pi)
    if sfwm_lmax:
      isiso = [ lm == 0 for lm in sfwm_lmax ]
    else:
      isiso = [ bv < bzero_threshold for bv in bvalues ]
    with open('ewmrf.txt', 'w', encoding='utf-8') as ewr:
      for iis in isiso:
        if iis:
          ewr.write(f'{refwmcoef} 0 0 0\n')
        else:
          ewr.write(f'{refwmcoef} {-refwmcoef} {refwmcoef} {-refwmcoef}\n')
    run.command('dwi2fod msmt_csd dwi.mif ewmrf.txt abs_ewm2.mif response_csf.txt abs_csf2.mif -mask refined_wm.mif -lmax 2,0' + bvalues_option, show=False)
    run.command('mrconvert abs_ewm2.mif - -coord 3 0 | mrcalc - abs_csf2.mif -add abs_sum2.mif', show=False)
    run.command('sh2peaks abs_ewm2.mif - -num 1 -mask refined_wm.mif | peaks2amp - - | mrcalc - abs_sum2.mif -divide - | mrconvert - metric_sfwm2.mif -coord 3 0 -axes 0,1,2', show=False)
    run.command(f'mrcalc refined_wm.mif metric_sfwm2.mif 0 -if - | mrthreshold - - -top {2*voxsfwmcount} -ignorezero | mrcalc refined_wm.mif - 0 -if - -datatype bit | mrconvert - refined_sfwm.mif -axes 0,1,2', show=False)
    run.command('dwi2fod msmt_csd dwi.mif ewmrf.txt abs_ewm6.mif response_csf.txt abs_csf6.mif -mask refined_sfwm.mif -lmax 6,0' + bvalues_option, show=False)
    run.command('mrconvert abs_ewm6.mif - -coord 3 0 | mrcalc - abs_csf6.mif -add abs_sum6.mif', show=False)
    run.command('sh2peaks abs_ewm6.mif - -num 1 -mask refined_sfwm.mif | peaks2amp - - | mrcalc - abs_sum6.mif -divide - | mrconvert - metric_sfwm6.mif -coord 3 0 -axes 0,1,2', show=False)
    run.command(f'mrcalc refined_sfwm.mif metric_sfwm6.mif 0 -if - | mrthreshold - - -top {voxsfwmcount} -ignorezero | mrcalc refined_sfwm.mif - 0 -if - -datatype bit | mrconvert - voxels_sfwm.mif -axes 0,1,2', show=False)

  statvoxsfwmcount = image.statistics('voxels_sfwm.mif', mask='voxels_sfwm.mif').count
  app.console(f'   [ WM: {statrefwmcount} -> {statvoxsfwmcount} (single-fibre) ]')
  # Estimate SF WM response function
  app.console(' * Estimating response function...')
  run.command('amp2response dwi.mif voxels_sfwm.mif safe_vecs.mif response_sfwm.txt' + bvalues_option + sfwm_lmax_option, show=False)


  # OUTPUT AND SUMMARY
  app.console('-------')
  app.console('Generating outputs...')

  # Generate 4D binary images with voxel selections at major stages in algorithm (RGB: WM=blue, GM=green, CSF=red).
  run.command('mrcat crude_csf.mif crude_gm.mif crude_wm.mif check_crude.mif -axis 3', show=False)
  run.command('mrcat refined_csf.mif refined_gm.mif refined_wm.mif check_refined.mif -axis 3', show=False)
  run.command('mrcat voxels_csf.mif voxels_gm.mif voxels_sfwm.mif check_voxels.mif -axis 3', show=False)

  # Copy results to output files
  run.function(shutil.copyfile, 'response_sfwm.txt', app.ARGS.out_sfwm, show=False)
  run.function(shutil.copyfile, 'response_gm.txt', app.ARGS.out_gm, show=False)
  run.function(shutil.copyfile, 'response_csf.txt', app.ARGS.out_csf, show=False)
  if app.ARGS.voxels:
    run.command(['mrconvert', 'check_voxels.mif', app.ARGS.voxels],
                mrconvert_keyval=app.ARGS.input,
                force=app.FORCE_OVERWRITE,
                preserve_pipes=True,
                show=False)
  app.console('-------')
