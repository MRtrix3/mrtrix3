def initParser(subparsers, base_parser):
  import argparse
  import lib.app
  lib.app.addCitation('If using \'tournier\' algorithm', 'Tournier, J.-D.; Calamante, F. & Connelly, A. Determination of the appropriate b value and number of gradient directions for high-angular-resolution diffusion-weighted imaging. NMR Biomedicine, 2013, 26, 1775-1786', False)
  parser = subparsers.add_parser('tournier', parents=[base_parser], add_help=False, description='Use the Tournier et al. (2013) iterative algorithm for single-fibre voxel selection and response function estimation')
  parser.add_argument('input', help='The input DWI')
  parser.add_argument('output', help='The output response function text file')
  options = parser.add_argument_group('Options specific to the \'tournier\' algorithm')
  options.add_argument('-iter_voxels', type=int, default=3000, help='Number of single-fibre voxels to select when preparing for the next iteration')
  options.add_argument('-sf_voxels', type=int, default=300, help='Number of single-fibre voxels to use when calculating response function')
  options.add_argument('-dilate', type=int, default=1, help='Number of mask dilation steps to apply when deriving voxel mask to test in the next iteration')
  options.add_argument('-max_iters', type=int, default=10, help='Maximum number of iterations')
  parser.set_defaults(algorithm='tournier')
  parser.set_defaults(single_shell=True)
  
  
  
def checkOutputFiles():
  import lib.app
  lib.app.checkOutputFile(lib.app.args.output)



def getInputFiles():
  pass



def execute():
  import os, shutil
  import lib.app
  from lib.delFile       import delFile
  from lib.getHeaderInfo import getHeaderInfo
  from lib.getImageStat  import getImageStat
  from lib.getUserPath   import getUserPath
  from lib.printMessage  import printMessage
  from lib.runCommand    import runCommand
  
  lmax_option = ''
  if lib.app.args.lmax:
    lmax_option = ' -lmax ' + lib.app.args.lmax

  if lib.app.args.max_iters < 2:
    errorMessage('Number of iterations must be at least 2')

  runCommand('amp2sh dwi.mif dwiSH.mif' + lmax_option)

  for iteration in range(0, lib.app.args.max_iters):
    prefix = 'iter' + str(iteration) + '_'
  
    if iteration == 0:
      RF_in_path = 'init_RF.txt'
      mask_in_path = 'mask.mif'
      init_RF = '1 -1 1'
      with open(RF_in_path, 'w') as f:
        f.write(init_RF);
      iter_lmax_option = ' -lmax 4'
    else:
      RF_in_path = 'iter' + str(iteration-1) + '_RF.txt'
      mask_in_path = 'iter' + str(iteration-1) + '_SF_dilated.mif'
      iter_lmax_option = lmax_option

    # Run CSD
    runCommand('dwi2fod csd dwi.mif ' + RF_in_path + ' ' + prefix + 'FOD.mif -mask ' + mask_in_path + iter_lmax_option)
    # Get amplitudes of two largest peaks, and direction of largest
    # TODO Speed-test fod2fixel against sh2peaks
    # TODO Add maximum number of fixels per voxel option to fod2fixel?
    runCommand('fod2fixel ' + prefix + 'FOD.mif -peak ' + prefix + 'peaks.msf -mask ' + mask_in_path + ' -fmls_no_thresholds')
    delFile(prefix + 'FOD.mif')
    if iteration:
      delFile(mask_in_path)
    runCommand('fixel2voxel ' + prefix + 'peaks.msf split_dir ' + prefix + 'all_dirs.mif')
    delFile(prefix + 'peaks.msf')
    runCommand('mrconvert ' + prefix + 'all_dirs.mif ' + prefix + 'first_dir.mif -coord 3 0:2')
    delFile(prefix + 'all_dirs.mif')
    runCommand('fixel2voxel ' + prefix + 'peaks.msf split_value ' + prefix + 'amps.mif')
    if [ int(size) for size in getHeaderInfo(prefix + 'amps.mif', 'size').split() ][3] == 1:
      if iteration:
        printMessage('No crosing fibres detected in / surrounding mask from iteration ' + str(iteration-1) + '; using response from that iteration')
        shutil.copyfile('iter' + str(iteration-1) + '_RF.txt', 'response.txt')
        shutil.copyfile('iter' + str(iteration-1) + '_SF.mif', 'voxels.mif')
        break
      else:
        warnMessage('No crossing fibres detected in input mask; using all input voxels for response function estimation')
        runCommand('sh2response dwiSH.mif mask.mif ' + prefix + 'first_dir.mif ' + prefix + 'response.txt' + iter_lmax_option)
        shutil.copyfile('mask.mif', 'voxels.mif')
        break
      
    runCommand('mrconvert ' + prefix + 'amps.mif ' + prefix + 'first_peaks.mif -coord 3 0 -axes 0,1,2')
    runCommand('mrconvert ' + prefix + 'amps.mif ' + prefix + 'second_peaks.mif -coord 3 1 -axes 0,1,2')
    delFile(prefix + 'amps.mif')
    # Calculate the 'cost function' Donald derived for selecting single-fibre voxels
    # https://github.com/MRtrix3/mrtrix3/pull/426
    #  sqrt(|peak1|) * (1 - |peak2| / |peak1|)^2
    runCommand('mrcalc ' + prefix + 'first_peaks.mif -sqrt 1 ' + prefix + 'second_peaks.mif ' + prefix + 'first_peaks.mif -div -sub 2 -pow -mult '+ prefix + 'CF.mif')
    delFile(prefix + 'first_peaks.mif')
    delFile(prefix + 'second_peaks.mif')
    # Select the top-ranked voxels
    runCommand('mrthreshold ' + prefix + 'CF.mif -top ' + str(lib.app.args.sf_voxels) + ' ' + prefix + 'SF.mif')
    # Generate a new response function based on this selection
    runCommand('sh2response dwiSH.mif ' + prefix + 'SF.mif ' + prefix + 'first_dir.mif ' + prefix + 'RF.txt' + iter_lmax_option)
    delFile(prefix + 'first_dir.mif')
    # Should we terminate?
    if iteration > 0:
      runCommand('mrcalc ' + prefix + 'SF.mif iter' + str(iteration-1) + '_SF.mif -sub ' + prefix + 'SF_diff.mif')
      delFile('iter' + str(iteration-1) + '_SF.mif')
      max_diff = getImageStat(prefix + 'SF_diff.mif', 'max')
      delFile(prefix + 'SF_diff.mif')
      if int(max_diff) == 0:
        printMessage('Convergence of SF voxel selection detected at iteration ' + str(iteration))
        delFile(prefix + 'CF.mif')
        shutil.copyfile(prefix + 'RF.txt', 'response.txt')
        shutil.copyfile(prefix + 'SF.mif', 'voxels.mif')
        break

    # Select a greater number of top single-fibre voxels, and dilate (within bounds of initial mask);
    #   these are the voxels that will be re-tested in the next iteration
    runCommand('mrthreshold ' + prefix + 'CF.mif -top ' + str(lib.app.args.iter_voxels) + ' - | maskfilter - dilate - -npass ' + str(lib.app.args.dilate) + ' | mrcalc mask.mif - -mult ' + prefix + 'SF_dilated.mif')
    delFile(prefix + 'CF.mif')

  # Commence the next iteration

  # If terminating due to running out of iterations, still need to put the results in the appropriate location
  if not os.path.exists('response.txt'):
    printMessage('Exiting after maximum ' + str(lib.app.args.max_iters) + ' iterations')
    shutil.copyfile('iter' + str(lib.app.args.max_iters-1) + '_RF.txt', 'response.txt')
    shutil.move('iter' + str(lib.app.args.max_iters-1) + '_SF.mif', 'voxels.mif')
    
  shutil.copyfile('response.txt', getUserPath(lib.app.args.output, False))

