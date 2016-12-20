def initParser(subparsers, base_parser):
  import argparse
  import lib.app
  lib.app.addCitation('If using \'dhollander\' algorithm', 'Dhollander, T.; Raffelt, D. & Connelly, A. Unsupervised 3-tissue response function estimation from single-shell or multi-shell diffusion MR data without a co-registered T1 image. ISMRM Workshop on Breaking the Barriers of Diffusion MRI, 2016, 5', False)
  parser = subparsers.add_parser('dhollander', parents=[base_parser], add_help=False, description='Unsupervised estimation of WM, GM and CSF response functions. Does not require a T1 image (or segmentation thereof).')
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
  parser.set_defaults(algorithm='dhollander')
  parser.set_defaults(single_shell=False)
  
  
  
def checkOutputFiles():
  import lib.app
  lib.app.checkOutputFile(lib.app.args.out_sfwm)
  lib.app.checkOutputFile(lib.app.args.out_gm)
  lib.app.checkOutputFile(lib.app.args.out_csf)
  
  
  
def getInputFiles():
  pass



def execute():
  import math, os, shutil
  import lib.app
  from lib.errorMessage  import errorMessage
  from lib.getHeaderInfo import getHeaderInfo
  from lib.getImageStat  import getImageStat
  from lib.getUserPath   import getUserPath
  from lib.printMessage  import printMessage
  from lib.runCommand    import runCommand
  from lib.runFunction   import runFunction
  from lib.warnMessage   import warnMessage

  


  # Get b-values and number of volumes per b-value.
  bvalues = [ int(round(float(x))) for x in getHeaderInfo('dwi.mif', 'shells').split() ]
  bvolumes = [ int(x) for x in getHeaderInfo('dwi.mif', 'shellcounts').split() ]
  printMessage(str(len(bvalues)) + ' unique b-value(s) detected: ' + ','.join(map(str,bvalues)) + ' with ' + ','.join(map(str,bvolumes)) + ' volumes.')
  if len(bvalues) < 2:
    errorMessage('Need at least 2 unique b-values (including b=0).')


  # Get lmax information (if provided).
  sfwm_lmax = [ ]
  if lib.app.args.lmax:
    sfwm_lmax = [ int(x.strip()) for x in lib.app.args.lmax.split(',') ]
    if not len(sfwm_lmax) == len(bvalues):
      errorMessage('Number of lmax\'s (' + str(len(sfwm_lmax)) + ', as supplied to the -lmax option: ' + ','.join(map(str,sfwm_lmax)) + ') does not match number of unique b-values.')
    for l in sfwm_lmax:
      if l%2:
        errorMessage('Values supplied to the -lmax option must be even.')
      if l<0:
        errorMessage('Values supplied to the -lmax option must be non-negative.')


  # Erode (brain) mask.
  if lib.app.args.erode > 0:
    runCommand('maskfilter mask.mif erode eroded_mask.mif -npass ' + str(lib.app.args.erode))
  else:
    runCommand('mrconvert mask.mif eroded_mask.mif -datatype bit')


  # Get volumes, compute mean signal and SDM per b-value; compute overall SDM; get rid of erroneous values.
  totvolumes = 0
  fullsdmcmd = 'mrcalc'
  errcmd = 'mrcalc'
  zeropath = 'mean_b' + str(bvalues[0]) + '.mif'
  for i, b in enumerate(bvalues):
    meanpath = 'mean_b' + str(b) + '.mif'
    runCommand('dwiextract dwi.mif -shell ' + str(b) + ' - | mrmath - mean ' + meanpath + ' -axis 3')
    errpath = 'err_b' + str(b) + '.mif'
    runCommand('mrcalc ' + meanpath + ' -finite ' + meanpath + ' 0 -if 0 -le ' + errpath + ' -datatype bit')
    errcmd += ' ' + errpath
    if i>0:
      errcmd += ' -add'
      sdmpath = 'sdm_b' + str(b) + '.mif'
      runCommand('mrcalc ' + zeropath + ' ' + meanpath +  ' -divide -log ' + sdmpath)
      totvolumes += bvolumes[i]
      fullsdmcmd += ' ' + sdmpath + ' ' + str(bvolumes[i]) + ' -mult'
      if i>1:
        fullsdmcmd += ' -add'
  fullsdmcmd += ' ' + str(totvolumes) + ' -divide full_sdm.mif'
  runCommand(fullsdmcmd)
  runCommand('mrcalc full_sdm.mif -finite full_sdm.mif 0 -if 0 -le err_sdm.mif -datatype bit')
  errcmd += ' err_sdm.mif -add 0 eroded_mask.mif -if safe_mask.mif -datatype bit'
  runCommand(errcmd)
  runCommand('mrcalc safe_mask.mif full_sdm.mif 0 -if 10 -min safe_sdm.mif')


  # Compute FA and principal eigenvectors; crude WM versus GM-CSF separation based on FA.
  runCommand('dwi2tensor dwi.mif - -mask safe_mask.mif | tensor2metric - -fa safe_fa.mif -vector safe_vecs.mif -modulate none -mask safe_mask.mif')
  runCommand('mrcalc safe_mask.mif safe_fa.mif 0 -if ' + str(lib.app.args.fa) + ' -gt crude_wm.mif -datatype bit')
  runCommand('mrcalc crude_wm.mif 0 safe_mask.mif -if _crudenonwm.mif -datatype bit')

  # Crude GM versus CSF separation based on SDM.
  crudenonwmmedian = getImageStat('safe_sdm.mif', 'median', '_crudenonwm.mif')
  runCommand('mrcalc _crudenonwm.mif safe_sdm.mif ' + str(crudenonwmmedian) + ' -subtract 0 -if - | mrthreshold - - -mask _crudenonwm.mif | mrcalc _crudenonwm.mif - 0 -if crude_csf.mif -datatype bit')
  runCommand('mrcalc crude_csf.mif 0 _crudenonwm.mif -if crude_gm.mif -datatype bit')


  # Refine WM: remove high SDM outliers.
  crudewmmedian = getImageStat('safe_sdm.mif', 'median', 'crude_wm.mif')
  runCommand('mrcalc crude_wm.mif safe_sdm.mif 0 -if ' + str(crudewmmedian) + ' -gt _crudewmhigh.mif -datatype bit')
  runCommand('mrcalc _crudewmhigh.mif 0 crude_wm.mif -if _crudewmlow.mif -datatype bit')
  crudewmQ1 = float(getImageStat('safe_sdm.mif', 'median', '_crudewmlow.mif'))
  crudewmQ3 = float(getImageStat('safe_sdm.mif', 'median', '_crudewmhigh.mif'))
  crudewmoutlthresh = crudewmQ3 + (crudewmQ3 - crudewmQ1)
  runCommand('mrcalc crude_wm.mif safe_sdm.mif 0 -if ' + str(crudewmoutlthresh) + ' -gt _crudewmoutliers.mif -datatype bit')
  runCommand('mrcalc _crudewmoutliers.mif 0 crude_wm.mif -if refined_wm.mif -datatype bit')

  # Refine GM: separate safer GM from partial volumed voxels.
  crudegmmedian = getImageStat('safe_sdm.mif', 'median', 'crude_gm.mif')
  runCommand('mrcalc crude_gm.mif safe_sdm.mif 0 -if ' + str(crudegmmedian) + ' -gt _crudegmhigh.mif -datatype bit')
  runCommand('mrcalc _crudegmhigh.mif 0 crude_gm.mif -if _crudegmlow.mif -datatype bit')
  runCommand('mrcalc _crudegmhigh.mif safe_sdm.mif ' + str(crudegmmedian) + ' -subtract 0 -if - | mrthreshold - - -mask _crudegmhigh.mif -invert | mrcalc _crudegmhigh.mif - 0 -if _crudegmhighselect.mif -datatype bit')
  runCommand('mrcalc _crudegmlow.mif safe_sdm.mif ' + str(crudegmmedian) + ' -subtract -neg 0 -if - | mrthreshold - - -mask _crudegmlow.mif -invert | mrcalc _crudegmlow.mif - 0 -if _crudegmlowselect.mif -datatype bit')
  runCommand('mrcalc _crudegmhighselect.mif 1 _crudegmlowselect.mif -if refined_gm.mif -datatype bit')

  # Refine CSF: recover lost CSF from crude WM SDM outliers, separate safer CSF from partial volumed voxels.
  crudecsfmin = getImageStat('safe_sdm.mif', 'min', 'crude_csf.mif')
  runCommand('mrcalc _crudewmoutliers.mif safe_sdm.mif 0 -if ' + str(crudecsfmin) + ' -gt 1 crude_csf.mif -if _crudecsfextra.mif -datatype bit')
  runCommand('mrcalc _crudecsfextra.mif safe_sdm.mif ' + str(crudecsfmin) + ' -subtract 0 -if - | mrthreshold - - -mask _crudecsfextra.mif | mrcalc _crudecsfextra.mif - 0 -if refined_csf.mif -datatype bit')


  # Get final voxels for single-fibre WM response function estimation from WM using 'tournier' algorithm.
  refwmcount = float(getImageStat('refined_wm.mif', 'count', 'refined_wm.mif'))
  voxsfwmcount = int(round(refwmcount * lib.app.args.sfwm / 100.0))
  printMessage('Running \'tournier\' algorithm to select ' + str(voxsfwmcount) + ' single-fibre WM voxels.')
  cleanopt = ''
  if not lib.app.cleanup:
    cleanopt = ' -nocleanup'
  runCommand('dwi2response tournier dwi.mif _respsfwmss.txt -sf_voxels ' + str(voxsfwmcount) + ' -iter_voxels ' + str(voxsfwmcount * 10) + ' -mask refined_wm.mif -voxels voxels_sfwm.mif -quiet -tempdir ' + lib.app.tempDir + cleanopt)

  # Get final voxels for GM response function estimation from GM.
  refgmmedian = getImageStat('safe_sdm.mif', 'median', 'refined_gm.mif')
  runCommand('mrcalc refined_gm.mif safe_sdm.mif 0 -if ' + str(refgmmedian) + ' -gt _refinedgmhigh.mif -datatype bit')
  runCommand('mrcalc _refinedgmhigh.mif 0 refined_gm.mif -if _refinedgmlow.mif -datatype bit')
  refgmhighcount = float(getImageStat('_refinedgmhigh.mif', 'count', '_refinedgmhigh.mif'))
  refgmlowcount = float(getImageStat('_refinedgmlow.mif', 'count', '_refinedgmlow.mif'))
  voxgmhighcount = int(round(refgmhighcount * lib.app.args.gm / 100.0))
  voxgmlowcount = int(round(refgmlowcount * lib.app.args.gm / 100.0))
  runCommand('mrcalc _refinedgmhigh.mif safe_sdm.mif 0 -if - | mrthreshold - - -bottom ' + str(voxgmhighcount) + ' -ignorezero | mrcalc _refinedgmhigh.mif - 0 -if _refinedgmhighselect.mif -datatype bit')
  runCommand('mrcalc _refinedgmlow.mif safe_sdm.mif 0 -if - | mrthreshold - - -top ' + str(voxgmlowcount) + ' -ignorezero | mrcalc _refinedgmlow.mif - 0 -if _refinedgmlowselect.mif -datatype bit')
  runCommand('mrcalc _refinedgmhighselect.mif 1 _refinedgmlowselect.mif -if voxels_gm.mif -datatype bit')

  # Get final voxels for CSF response function estimation from CSF.
  refcsfcount = float(getImageStat('refined_csf.mif', 'count', 'refined_csf.mif'))
  voxcsfcount = int(round(refcsfcount * lib.app.args.csf / 100.0))
  runCommand('mrcalc refined_csf.mif safe_sdm.mif 0 -if - | mrthreshold - - -top ' + str(voxcsfcount) + ' -ignorezero | mrcalc refined_csf.mif - 0 -if voxels_csf.mif -datatype bit')


  # Show summary of voxels counts.
  textarrow = ' --> '
  printMessage('Summary of voxel counts:')
  printMessage('Mask: ' + str(int(getImageStat('mask.mif', 'count', 'mask.mif'))) + textarrow + str(int(getImageStat('eroded_mask.mif', 'count', 'eroded_mask.mif'))) + textarrow + str(int(getImageStat('safe_mask.mif', 'count', 'safe_mask.mif'))))
  printMessage('WM: ' + str(int(getImageStat('crude_wm.mif', 'count', 'crude_wm.mif'))) + textarrow + str(int(getImageStat('refined_wm.mif', 'count', 'refined_wm.mif'))) + textarrow + str(int(getImageStat('voxels_sfwm.mif', 'count', 'voxels_sfwm.mif'))) + ' (SF)')
  printMessage('GM: ' + str(int(getImageStat('crude_gm.mif', 'count', 'crude_gm.mif'))) + textarrow + str(int(getImageStat('refined_gm.mif', 'count', 'refined_gm.mif'))) + textarrow + str(int(getImageStat('voxels_gm.mif', 'count', 'voxels_gm.mif'))))
  printMessage('CSF: ' + str(int(getImageStat('crude_csf.mif', 'count', 'crude_csf.mif'))) + textarrow + str(int(getImageStat('refined_csf.mif', 'count', 'refined_csf.mif'))) + textarrow + str(int(getImageStat('voxels_csf.mif', 'count', 'voxels_csf.mif'))))


  # Generate single-fibre WM, GM and CSF responses
  bvalues_option = ' -shell ' + ','.join(map(str,bvalues))
  sfwm_lmax_option = ''
  if sfwm_lmax:
    sfwm_lmax_option = ' -lmax ' + ','.join(map(str,sfwm_lmax))
  runCommand('amp2response dwi.mif voxels_sfwm.mif safe_vecs.mif response_sfwm.txt' + bvalues_option + sfwm_lmax_option)
  runCommand('amp2response dwi.mif voxels_gm.mif safe_vecs.mif response_gm.txt' + bvalues_option + ' -isotropic')
  runCommand('amp2response dwi.mif voxels_csf.mif safe_vecs.mif response_csf.txt' + bvalues_option + ' -isotropic')
  runFunction(shutil.copyfile, 'response_sfwm.txt', getUserPath(lib.app.args.out_sfwm, False))
  runFunction(shutil.copyfile, 'response_gm.txt', getUserPath(lib.app.args.out_gm, False))
  runFunction(shutil.copyfile, 'response_csf.txt', getUserPath(lib.app.args.out_csf, False))


  # Generate 4D binary images with voxel selections at major stages in algorithm (RGB as in MSMT-CSD paper).
  runCommand('mrcat crude_csf.mif crude_gm.mif crude_wm.mif crude.mif -axis 3')
  runCommand('mrcat refined_csf.mif refined_gm.mif refined_wm.mif refined.mif -axis 3')
  runCommand('mrcat voxels_csf.mif voxels_gm.mif voxels_sfwm.mif voxels.mif -axis 3')


