def initialise(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('dhollander', author='Thijs Dhollander (thijs.dhollander@gmail.com)', synopsis='Use the Dhollander et al. (2016) algorithm for unsupervised estimation of WM, GM and CSF response functions; does not require a T1 image (or segmentation thereof)', parents=[base_parser])
  parser.addCitation('', 'Dhollander, T.; Raffelt, D. & Connelly, A. Unsupervised 3-tissue response function estimation from single-shell or multi-shell diffusion MR data without a co-registered T1 image. ISMRM Workshop on Breaking the Barriers of Diffusion MRI, 2016, 5', False)
  parser.addCitation('', 'Dhollander, T.; Raffelt, D. & Connelly, A. Accuracy of response function estimation algorithms for 3-tissue spherical deconvolution of diverse quality diffusion MRI data. Proc Intl Soc Mag Reson Med, 2018, 26, 1569', False)
  parser.add_argument('input', help='The input DWI')
  parser.add_argument('out_sfwm', help='Output single-fibre WM response text file')
  parser.add_argument('out_gm', help='Output GM response text file')
  parser.add_argument('out_csf', help='Output CSF response text file')
  options = parser.add_argument_group('Options specific to the \'dhollander\' algorithm')
  options.add_argument('-erode', type=int, default=3, help='Number of erosion passes to apply to initial (whole brain) mask. (default: 3)')
  options.add_argument('-fa', type=float, default=0.2, help='FA threshold for crude WM versus GM-CSF separation. (default: 0.2)')
  options.add_argument('-sfwm', type=float, default=0.5, help='Number of single-fibre WM voxels to select, as a percentage of refined WM. (default: 0.5 per cent)')
  options.add_argument('-gm', type=float, default=2.0, help='Number of GM voxels to select, as a percentage of refined GM. (default: 2 per cent)')
  options.add_argument('-csf', type=float, default=10.0, help='Number of CSF voxels to select, as a percentage of refined CSF. (default: 10 per cent)')



def checkOutputPaths(): #pylint: disable=unused-variable
  from mrtrix3 import app
  app.checkOutputPath(app.args.out_sfwm)
  app.checkOutputPath(app.args.out_gm)
  app.checkOutputPath(app.args.out_csf)



def getInputs(): #pylint: disable=unused-variable
  pass



def needsSingleShell(): #pylint: disable=unused-variable
  return False



def execute(): #pylint: disable=unused-variable
  import shutil
  from mrtrix3 import app, image, path, run


  # Get b-values and number of volumes per b-value.
  bvalues = [ int(round(float(x))) for x in image.mrinfo('dwi.mif', 'shell_bvalues').split() ]
  bvolumes = [ int(x) for x in image.mrinfo('dwi.mif', 'shell_sizes').split() ]
  app.console(str(len(bvalues)) + ' unique b-value(s) detected: ' + ','.join(map(str,bvalues)) + ' with ' + ','.join(map(str,bvolumes)) + ' volumes.')
  if len(bvalues) < 2:
    app.error('Need at least 2 unique b-values (including b=0).')


  # Get lmax information (if provided).
  sfwm_lmax = [ ]
  if app.args.lmax:
    sfwm_lmax = [ int(x.strip()) for x in app.args.lmax.split(',') ]
    if not len(sfwm_lmax) == len(bvalues):
      app.error('Number of lmax\'s (' + str(len(sfwm_lmax)) + ', as supplied to the -lmax option: ' + ','.join(map(str,sfwm_lmax)) + ') does not match number of unique b-values.')
    for l in sfwm_lmax:
      if l%2:
        app.error('Values supplied to the -lmax option must be even.')
      if l<0:
        app.error('Values supplied to the -lmax option must be non-negative.')


  # Erode (brain) mask.
  if app.args.erode > 0:
    run.command('maskfilter mask.mif erode eroded_mask.mif -npass ' + str(app.args.erode))
  else:
    run.command('mrconvert mask.mif eroded_mask.mif -datatype bit')


  # Get volumes, compute mean signal and SDM per b-value; compute overall SDM; get rid of erroneous values.
  totvolumes = 0
  fullsdmcmd = 'mrcalc'
  errcmd = 'mrcalc'
  zeropath = 'mean_b' + str(bvalues[0]) + '.mif'
  for i, b in enumerate(bvalues):
    meanpath = 'mean_b' + str(b) + '.mif'
    run.command('dwiextract dwi.mif -shells ' + str(b) + ' - | mrmath - mean ' + meanpath + ' -axis 3')
    errpath = 'err_b' + str(b) + '.mif'
    run.command('mrcalc ' + meanpath + ' -finite ' + meanpath + ' 0 -if 0 -le ' + errpath + ' -datatype bit')
    errcmd += ' ' + errpath
    if i>0:
      errcmd += ' -add'
      sdmpath = 'sdm_b' + str(b) + '.mif'
      run.command('mrcalc ' + zeropath + ' ' + meanpath +  ' -divide -log ' + sdmpath)
      totvolumes += bvolumes[i]
      fullsdmcmd += ' ' + sdmpath + ' ' + str(bvolumes[i]) + ' -mult'
      if i>1:
        fullsdmcmd += ' -add'
  fullsdmcmd += ' ' + str(totvolumes) + ' -divide full_sdm.mif'
  run.command(fullsdmcmd)
  run.command('mrcalc full_sdm.mif -finite full_sdm.mif 0 -if 0 -le err_sdm.mif -datatype bit')
  errcmd += ' err_sdm.mif -add 0 eroded_mask.mif -if safe_mask.mif -datatype bit'
  run.command(errcmd)
  run.command('mrcalc safe_mask.mif full_sdm.mif 0 -if 10 -min safe_sdm.mif')


  # Compute FA and principal eigenvectors; crude WM versus GM-CSF separation based on FA.
  run.command('dwi2tensor dwi.mif - -mask safe_mask.mif | tensor2metric - -fa safe_fa.mif -vector safe_vecs.mif -modulate none -mask safe_mask.mif')
  run.command('mrcalc safe_mask.mif safe_fa.mif 0 -if ' + str(app.args.fa) + ' -gt crude_wm.mif -datatype bit')
  run.command('mrcalc crude_wm.mif 0 safe_mask.mif -if _crudenonwm.mif -datatype bit')

  # Crude GM versus CSF separation based on SDM.
  crudenonwmmedian = image.statistic('safe_sdm.mif', 'median', '-mask _crudenonwm.mif')
  run.command('mrcalc _crudenonwm.mif safe_sdm.mif ' + str(crudenonwmmedian) + ' -subtract 0 -if - | mrthreshold - - -mask _crudenonwm.mif | mrcalc _crudenonwm.mif - 0 -if crude_csf.mif -datatype bit')
  run.command('mrcalc crude_csf.mif 0 _crudenonwm.mif -if crude_gm.mif -datatype bit')


  # Refine WM: remove high SDM outliers.
  crudewmmedian = image.statistic('safe_sdm.mif', 'median', '-mask crude_wm.mif')
  run.command('mrcalc crude_wm.mif safe_sdm.mif 0 -if ' + str(crudewmmedian) + ' -gt _crudewmhigh.mif -datatype bit')
  run.command('mrcalc _crudewmhigh.mif 0 crude_wm.mif -if _crudewmlow.mif -datatype bit')
  crudewmQ1 = float(image.statistic('safe_sdm.mif', 'median', '-mask _crudewmlow.mif'))
  crudewmQ3 = float(image.statistic('safe_sdm.mif', 'median', '-mask _crudewmhigh.mif'))
  crudewmoutlthresh = crudewmQ3 + (crudewmQ3 - crudewmQ1)
  run.command('mrcalc crude_wm.mif safe_sdm.mif 0 -if ' + str(crudewmoutlthresh) + ' -gt _crudewmoutliers.mif -datatype bit')
  run.command('mrcalc _crudewmoutliers.mif 0 crude_wm.mif -if refined_wm.mif -datatype bit')

  # Refine GM: separate safer GM from partial volumed voxels.
  crudegmmedian = image.statistic('safe_sdm.mif', 'median', '-mask crude_gm.mif')
  run.command('mrcalc crude_gm.mif safe_sdm.mif 0 -if ' + str(crudegmmedian) + ' -gt _crudegmhigh.mif -datatype bit')
  run.command('mrcalc _crudegmhigh.mif 0 crude_gm.mif -if _crudegmlow.mif -datatype bit')
  run.command('mrcalc _crudegmhigh.mif safe_sdm.mif ' + str(crudegmmedian) + ' -subtract 0 -if - | mrthreshold - - -mask _crudegmhigh.mif -invert | mrcalc _crudegmhigh.mif - 0 -if _crudegmhighselect.mif -datatype bit')
  run.command('mrcalc _crudegmlow.mif safe_sdm.mif ' + str(crudegmmedian) + ' -subtract -neg 0 -if - | mrthreshold - - -mask _crudegmlow.mif -invert | mrcalc _crudegmlow.mif - 0 -if _crudegmlowselect.mif -datatype bit')
  run.command('mrcalc _crudegmhighselect.mif 1 _crudegmlowselect.mif -if refined_gm.mif -datatype bit')

  # Refine CSF: recover lost CSF from crude WM SDM outliers, separate safer CSF from partial volumed voxels.
  crudecsfmin = image.statistic('safe_sdm.mif', 'min', '-mask crude_csf.mif')
  run.command('mrcalc _crudewmoutliers.mif safe_sdm.mif 0 -if ' + str(crudecsfmin) + ' -gt 1 crude_csf.mif -if _crudecsfextra.mif -datatype bit')
  run.command('mrcalc _crudecsfextra.mif safe_sdm.mif ' + str(crudecsfmin) + ' -subtract 0 -if - | mrthreshold - - -mask _crudecsfextra.mif | mrcalc _crudecsfextra.mif - 0 -if refined_csf.mif -datatype bit')


  # Get final voxels for single-fibre WM response function estimation from WM using 'tournier' algorithm.
  refwmcount = float(image.statistic('refined_wm.mif', 'count', '-mask refined_wm.mif'))
  voxsfwmcount = int(round(refwmcount * app.args.sfwm / 100.0))
  app.console('Running \'tournier\' algorithm to select ' + str(voxsfwmcount) + ' single-fibre WM voxels.')
  cleanopt = ''
  if not app.cleanup:
    cleanopt = ' -nocleanup'
  run.command('dwi2response tournier dwi.mif _respsfwmss.txt -sf_voxels ' + str(voxsfwmcount) + ' -iter_voxels ' + str(voxsfwmcount * 10) + ' -mask refined_wm.mif -voxels voxels_sfwm.mif -tempdir ' + app.tempDir + cleanopt)

  # Get final voxels for GM response function estimation from GM.
  refgmmedian = image.statistic('safe_sdm.mif', 'median', '-mask refined_gm.mif')
  run.command('mrcalc refined_gm.mif safe_sdm.mif 0 -if ' + str(refgmmedian) + ' -gt _refinedgmhigh.mif -datatype bit')
  run.command('mrcalc _refinedgmhigh.mif 0 refined_gm.mif -if _refinedgmlow.mif -datatype bit')
  refgmhighcount = float(image.statistic('_refinedgmhigh.mif', 'count', '-mask _refinedgmhigh.mif'))
  refgmlowcount = float(image.statistic('_refinedgmlow.mif', 'count', '-mask _refinedgmlow.mif'))
  voxgmhighcount = int(round(refgmhighcount * app.args.gm / 100.0))
  voxgmlowcount = int(round(refgmlowcount * app.args.gm / 100.0))
  run.command('mrcalc _refinedgmhigh.mif safe_sdm.mif 0 -if - | mrthreshold - - -bottom ' + str(voxgmhighcount) + ' -ignorezero | mrcalc _refinedgmhigh.mif - 0 -if _refinedgmhighselect.mif -datatype bit')
  run.command('mrcalc _refinedgmlow.mif safe_sdm.mif 0 -if - | mrthreshold - - -top ' + str(voxgmlowcount) + ' -ignorezero | mrcalc _refinedgmlow.mif - 0 -if _refinedgmlowselect.mif -datatype bit')
  run.command('mrcalc _refinedgmhighselect.mif 1 _refinedgmlowselect.mif -if voxels_gm.mif -datatype bit')

  # Get final voxels for CSF response function estimation from CSF.
  refcsfcount = float(image.statistic('refined_csf.mif', 'count', '-mask refined_csf.mif'))
  voxcsfcount = int(round(refcsfcount * app.args.csf / 100.0))
  run.command('mrcalc refined_csf.mif safe_sdm.mif 0 -if - | mrthreshold - - -top ' + str(voxcsfcount) + ' -ignorezero | mrcalc refined_csf.mif - 0 -if voxels_csf.mif -datatype bit')


  # Show summary of voxels counts.
  textarrow = ' --> '
  app.console('Summary of voxel counts:')
  app.console('Mask: ' + str(int(image.statistic('mask.mif', 'count', '-mask mask.mif'))) + textarrow + str(int(image.statistic('eroded_mask.mif', 'count', '-mask eroded_mask.mif'))) + textarrow + str(int(image.statistic('safe_mask.mif', 'count', '-mask safe_mask.mif'))))
  app.console('WM: ' + str(int(image.statistic('crude_wm.mif', 'count', '-mask crude_wm.mif'))) + textarrow + str(int(image.statistic('refined_wm.mif', 'count', '-mask refined_wm.mif'))) + textarrow + str(int(image.statistic('voxels_sfwm.mif', 'count', '-mask voxels_sfwm.mif'))) + ' (SF)')
  app.console('GM: ' + str(int(image.statistic('crude_gm.mif', 'count', '-mask crude_gm.mif'))) + textarrow + str(int(image.statistic('refined_gm.mif', 'count', '-mask refined_gm.mif'))) + textarrow + str(int(image.statistic('voxels_gm.mif', 'count', '-mask voxels_gm.mif'))))
  app.console('CSF: ' + str(int(image.statistic('crude_csf.mif', 'count', '-mask crude_csf.mif'))) + textarrow + str(int(image.statistic('refined_csf.mif', 'count', '-mask refined_csf.mif'))) + textarrow + str(int(image.statistic('voxels_csf.mif', 'count', '-mask voxels_csf.mif'))))


  # Generate single-fibre WM, GM and CSF responses
  bvalues_option = ' -shells ' + ','.join(map(str,bvalues))
  sfwm_lmax_option = ''
  if sfwm_lmax:
    sfwm_lmax_option = ' -lmax ' + ','.join(map(str,sfwm_lmax))
  run.command('amp2response dwi.mif voxels_sfwm.mif safe_vecs.mif response_sfwm.txt' + bvalues_option + sfwm_lmax_option)
  run.command('amp2response dwi.mif voxels_gm.mif safe_vecs.mif response_gm.txt' + bvalues_option + ' -isotropic')
  run.command('amp2response dwi.mif voxels_csf.mif safe_vecs.mif response_csf.txt' + bvalues_option + ' -isotropic')
  run.function(shutil.copyfile, 'response_sfwm.txt', path.fromUser(app.args.out_sfwm, False))
  run.function(shutil.copyfile, 'response_gm.txt', path.fromUser(app.args.out_gm, False))
  run.function(shutil.copyfile, 'response_csf.txt', path.fromUser(app.args.out_csf, False))


  # Generate 4D binary images with voxel selections at major stages in algorithm (RGB as in MSMT-CSD paper).
  run.command('mrcat crude_csf.mif crude_gm.mif crude_wm.mif crude.mif -axis 3')
  run.command('mrcat refined_csf.mif refined_gm.mif refined_wm.mif refined.mif -axis 3')
  run.command('mrcat voxels_csf.mif voxels_gm.mif voxels_sfwm.mif voxels.mif -axis 3')
