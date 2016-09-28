def initParser(subparsers, base_parser):
  import argparse
  import lib.app
  lib.app.addCitation('If using \'msmt_csd\' algorithm', 'Jeurissen, B.; Tournier, J.-D.; Dhollander, T.; Connelly, A. & Sijbers, J. Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data. NeuroImage, 2014, 103, 411-426', False)
  parser = subparsers.add_parser('msmt_5tt', parents=[base_parser], add_help=False, description='Derive MSMT-CSD tissue response functions based on a co-registered five-tissue-type (5TT) image')
  parser.add_argument('input', help='The input DWI')
  parser.add_argument('in_5tt', help='Input co-registered 5TT image')
  parser.add_argument('out_wm', help='Output WM response text file')
  parser.add_argument('out_gm', help='Output GM response text file')
  parser.add_argument('out_csf', help='Output CSF response text file')
  options = parser.add_argument_group('Options specific to the \'msmt_5tt\' algorithm')
  options.add_argument('-dirs', help='Manually provide the fibre direction in each voxel (a tensor fit will be used otherwise)')
  options.add_argument('-fa', type=float, default=0.2, help='Upper fractional anisotropy threshold for GM and CSF voxel selection')
  options.add_argument('-pvf', type=float, default=0.95, help='Partial volume fraction threshold for tissue voxel selection')
  options.add_argument('-wm_algo', metavar='algorithm', default='tournier', help='dwi2response algorithm to use for WM single-fibre voxel selection')
  parser.set_defaults(algorithm='msmt_5tt')
  parser.set_defaults(single_shell=False)
  
  
  
def checkOutputFiles():
  import lib.app
  lib.app.checkOutputFile(lib.app.args.out_wm)
  lib.app.checkOutputFile(lib.app.args.out_gm)
  lib.app.checkOutputFile(lib.app.args.out_csf)



def getInputFiles():
  import os
  import lib.app
  from lib.getUserPath import getUserPath
  from lib.runCommand  import runCommand
  runCommand('mrconvert ' + getUserPath(lib.app.args.in_5tt, True) + ' ' + os.path.join(lib.app.tempDir, '5tt.mif'))
  if lib.app.args.dirs:
    runCommand('mrconvert ' + getUserPath(lib.app.args.dirs, True) + ' ' + os.path.join(lib.app.tempDir, 'dirs.mif') + ' -stride 0,0,0,1')



def execute():
  import math, os, shutil
  import lib.app
  from lib.getHeaderInfo import getHeaderInfo
  from lib.getImageStat  import getImageStat
  from lib.getUserPath   import getUserPath
  from lib.printMessage  import printMessage
  from lib.runCommand    import runCommand
  from lib.warnMessage   import warnMessage
  from lib.errorMessage  import errorMessage
  
  # Ideally want to use the oversampling-based regridding of the 5TT image from the SIFT model, not mrtransform
  # May need to commit 5ttregrid...

  # Verify input 5tt image
  runCommand('5ttcheck 5tt.mif')

  # Get shell information
  shells = [ int(round(float(x))) for x in getHeaderInfo('dwi.mif', 'shells').split() ]
  if len(shells) < 3:
    warnMessage('Less than three b-value shells; response functions will not be applicable in MSMT-CSD algorithm')

  # Get lmax information (if provided)
  wm_lmax = [ ]
  if lib.app.args.lmax:
    wm_lmax = [ int(x.strip()) for x in lib.app.args.lmax.split(',') ]
    if not len(wm_lmax) == len(shells):
      errorMessage('Number of manually-defined lmax\'s (' + str(len(wm_lmax)) + ') does not match number of b-value shells (' + str(len(shells)) + ')')
    for l in wm_lmax:
      if l%2:
        errorMessage('Values for lmax must be even')
      if l<0:
        errorMessage('Values for lmax must be non-negative')

  runCommand('dwi2tensor dwi.mif - -mask mask.mif | tensor2metric - -fa fa.mif -vector vector.mif')
  if not os.path.exists('dirs.mif'):
    shutil.copy('vector.mif', 'dirs.mif')
  runCommand('mrtransform 5tt.mif 5tt_regrid.mif -template fa.mif -interp linear')

  # Basic tissue masks
  runCommand('mrconvert 5tt_regrid.mif - -coord 3 2 -axes 0,1,2 | mrcalc - ' + str(lib.app.args.pvf) + ' -gt mask.mif -mult wm_mask.mif')
  runCommand('mrconvert 5tt_regrid.mif - -coord 3 0 -axes 0,1,2 | mrcalc - ' + str(lib.app.args.pvf) + ' -gt fa.mif ' + str(lib.app.args.fa) + ' -lt -mult mask.mif -mult gm_mask.mif')
  runCommand('mrconvert 5tt_regrid.mif - -coord 3 3 -axes 0,1,2 | mrcalc - ' + str(lib.app.args.pvf) + ' -gt fa.mif ' + str(lib.app.args.fa) + ' -lt -mult mask.mif -mult csf_mask.mif')

  # Revise WM mask to only include single-fibre voxels
  printMessage('Calling dwi2response recursively to select WM single-fibre voxels using \'' + lib.app.args.wm_algo + '\' algorithm')
  recursive_cleanup_option=''
  if not lib.app.cleanup:
    recursive_cleanup_option = ' -nocleanup'
  runCommand('dwi2response ' + lib.app.args.wm_algo + ' dwi.mif wm_ss_response.txt -mask wm_mask.mif -voxels wm_sf_mask.mif -quiet -tempdir ' + lib.app.tempDir + recursive_cleanup_option)

  # Check for empty masks
  wm_voxels  = int(getImageStat('wm_sf_mask.mif', 'count', 'wm_sf_mask.mif'))
  gm_voxels  = int(getImageStat('gm_mask.mif',    'count', 'gm_mask.mif'))
  csf_voxels = int(getImageStat('csf_mask.mif',   'count', 'csf_mask.mif'))
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
    errorMessage(message)

  # For each of the three tissues, generate a multi-shell response
  # Since here we're guaranteeing that GM and CSF will be isotropic in all shells, let's use mrstats rather than sh2response (seems a bit weird passing a directions file to sh2response with lmax=0...)

  wm_responses  = [ ]
  gm_responses  = [ ]
  csf_responses = [ ]
  max_length = 0

  for index, b in enumerate(shells):
    dwi_path = 'dwi_b' + str(b) + '.mif'
    # dwiextract will yield a 4D image, even if there's only a single volume in a shell
    runCommand('dwiextract dwi.mif -shell ' + str(b) + ' ' + dwi_path)
    this_b_lmax_option = ''
    if wm_lmax:
      this_b_lmax_option = ' -lmax ' + str(wm_lmax[index])
    runCommand('amp2sh ' + dwi_path + ' - | sh2response - wm_sf_mask.mif dirs.mif wm_response_b' + str(b) + '.txt' + this_b_lmax_option)
    wm_response = open('wm_response_b' + str(b) + '.txt', 'r').read().split()
    wm_responses.append(wm_response)
    max_length = max(max_length, len(wm_response))
    mean_dwi_path = 'dwi_b' + str(b) + '_mean.mif'
    runCommand('mrmath ' + dwi_path + ' mean ' + mean_dwi_path + ' -axis 3')
    gm_mean  = float(getImageStat(mean_dwi_path, 'mean', 'gm_mask.mif'))
    csf_mean = float(getImageStat(mean_dwi_path, 'mean', 'csf_mask.mif'))
    gm_responses .append( str(gm_mean  * math.sqrt(4.0 * math.pi)) )
    csf_responses.append( str(csf_mean * math.sqrt(4.0 * math.pi)) )

  with open('wm.txt', 'w') as f:
    for line in wm_responses:
      line += ['0'] * (max_length - len(line))
      f.write(' '.join(line) + '\n')
  with open('gm.txt', 'w') as f:
    for line in gm_responses:
      f.write(line + '\n')
  with open('csf.txt', 'w') as f:
    for line in csf_responses:
      f.write(line + '\n')

  shutil.copyfile('wm.txt',  getUserPath(lib.app.args.out_wm,  False))
  shutil.copyfile('gm.txt',  getUserPath(lib.app.args.out_gm,  False))
  shutil.copyfile('csf.txt', getUserPath(lib.app.args.out_csf, False))

  # Generate output 4D binary image with voxel selections; RGB as in MSMT-CSD paper
  runCommand('mrcat csf_mask.mif gm_mask.mif wm_sf_mask.mif voxels.mif -axis 3')

