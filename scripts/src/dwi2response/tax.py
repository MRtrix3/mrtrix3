def initParser(subparsers, base_parser):
  import argparse
  import lib.app
  lib.app.addCitation('If using \'tax\' algorithm', 'Tax, C. M.; Jeurissen, B.; Vos, S. B.; Viergever, M. A. & Leemans, A. Recursive calibration of the fiber response function for spherical deconvolution of diffusion MRI data. NeuroImage, 2014, 86, 67-80', False)
  parser = subparsers.add_parser('tax', parents=[base_parser], add_help=False, description='Use the Tax et al. (2014) recursive calibration algorithm for single-fibre voxel selection and response function estimation')
  parser.add_argument('input', help='The input DWI')
  parser.add_argument('output', help='The output response function text file')
  options = parser.add_argument_group('Options specific to the \'tax\' algorithm')
  options.add_argument('-peak_ratio', type=float, default=0.1, help='Second-to-first-peak amplitude ratio threshold')
  options.add_argument('-max_iters', type=int, default=20, help='Maximum number of iterations')
  options.add_argument('-convergence', type=float, default=0.5, help='Percentile change in any RF coefficient required to continue iterating')
  parser.set_defaults(algorithm='tax')
  parser.set_defaults(single_shell=True)
  
  
  
def checkOutputFiles():
  import lib.app
  lib.app.checkOutputFile(lib.app.args.output)



def getInputFiles():
  pass



def execute():
  import math, os, shutil
  import lib.app
  import lib.file
  import lib.image
  import lib.message
  import lib.path
  from lib.runCommand  import runCommand
  from lib.runFunction import runFunction
  
  lmax_option = ''
  if lib.app.args.lmax:
    lmax_option = ' -lmax ' + lib.app.args.lmax

  convergence_change = 0.01 * lib.app.args.convergence

  for iteration in range(0, lib.app.args.max_iters):
    prefix = 'iter' + str(iteration) + '_'

    # How to initialise response function?
    # old dwi2response command used mean & standard deviation of DWI data; however
    #   this may force the output FODs to lmax=2 at the first iteration
    # Chantal used a tensor with low FA, but it'd be preferable to get the scaling right
    # Other option is to do as before, but get the ratio between l=0 and l=2, and
    #   generate l=4,6,... using that amplitude ratio
    if iteration == 0:
      RF_in_path = 'init_RF.txt'
      mask_in_path = 'mask.mif'
      # TODO This can be changed once #71 is implemented (mrstats statistics across volumes)
      volume_means = [float(x) for x in lib.image.statistic('dwi.mif', 'mean', 'mask.mif').split()]
      mean = sum(volume_means) / float(len(volume_means))
      volume_stds = [float(x) for x in lib.image.statistic('dwi.mif', 'std', 'mask.mif').split()]
      std = sum(volume_stds) / float(len(volume_stds))
      # Scale these to reflect the fact that we're moving to the SH basis
      mean *= math.sqrt(4.0 * math.pi)
      std  *= math.sqrt(4.0 * math.pi)
      # Now produce the initial response function
      # Let's only do it to lmax 4
      init_RF = [ str(mean), str(-0.5*std), str(0.25*std*std/mean) ]
      with open('init_RF.txt', 'w') as f:
        f.write(' '.join(init_RF))
    else:
      RF_in_path = 'iter' + str(iteration-1) + '_RF.txt'
      mask_in_path = 'iter' + str(iteration-1) + '_SF.mif'
  
    # Run CSD
    runCommand('dwi2fod csd dwi.mif ' + RF_in_path + ' ' + prefix + 'FOD.mif -mask ' + mask_in_path)
    # Get amplitudes of two largest peaks, and directions of largest
    runCommand('fod2fixel ' + prefix + 'FOD.mif ' + prefix + 'fixel -peak peaks.mif -mask ' + mask_in_path + ' -fmls_no_thresholds')
    lib.file.delTempFile(prefix + 'FOD.mif')
    runCommand('fixel2voxel ' + prefix + 'fixel/peaks.mif split_data ' + prefix + 'amps.mif')
    runCommand('mrconvert ' + prefix + 'amps.mif ' + prefix + 'first_peaks.mif -coord 3 0 -axes 0,1,2')
    runCommand('mrconvert ' + prefix + 'amps.mif ' + prefix + 'second_peaks.mif -coord 3 1 -axes 0,1,2')
    lib.file.delTempFile(prefix + 'amps.mif')
    runCommand('fixel2voxel ' + prefix + 'fixel/directions.mif split_dir ' + prefix + 'all_dirs.mif')
    lib.file.delTempFolder(prefix + 'fixel')
    runCommand('mrconvert ' + prefix + 'all_dirs.mif ' + prefix + 'first_dir.mif -coord 3 0:2')
    lib.file.delTempFile(prefix + 'all_dirs.mif')
    # Revise single-fibre voxel selection based on ratio of tallest to second-tallest peak
    runCommand('mrcalc ' + prefix + 'second_peaks.mif ' + prefix + 'first_peaks.mif -div ' + prefix + 'peak_ratio.mif')
    lib.file.delTempFile(prefix + 'first_peaks.mif')
    lib.file.delTempFile(prefix + 'second_peaks.mif')
    runCommand('mrcalc ' + prefix + 'peak_ratio.mif ' + str(lib.app.args.peak_ratio) + ' -lt ' + mask_in_path + ' -mult ' + prefix + 'SF.mif -datatype bit')
    lib.file.delTempFile(prefix + 'peak_ratio.mif')
    # Make sure image isn't empty
    SF_voxel_count = int(lib.image.statistic(prefix + 'SF.mif', 'count', prefix + 'SF.mif'))
    if not SF_voxel_count:
      lib.message.error('Aborting: All voxels have been excluded from single-fibre selection')
    # Generate a new response function
    runCommand('amp2response dwi.mif ' + prefix + 'SF.mif ' + prefix + 'first_dir.mif ' + prefix + 'RF.txt' + lmax_option)
    lib.file.delTempFile(prefix + 'first_dir.mif')
    
    # Detect convergence
    # Look for a change > some percentage - don't bother looking at the masks
    if iteration > 0:
      with open(RF_in_path, 'r') as old_RF_file:
        old_RF = [ float(x) for x in old_RF_file.read().split() ]
      with open(prefix + 'RF.txt', 'r') as new_RF_file:
        new_RF = [ float(x) for x in new_RF_file.read().split() ]
      reiterate = False
      for index in range(0, len(old_RF)):
        mean = 0.5 * (old_RF[index] + new_RF[index])
        diff = math.fabs(0.5 * (old_RF[index] - new_RF[index]))
        ratio = diff / mean
        if ratio > convergence_change:
          reiterate = True
      if not reiterate:
        lib.message.console('Exiting at iteration ' + str(iteration) + ' with ' + str(SF_voxel_count) + ' SF voxels due to unchanged response function coefficients')
        runFunction(shutil.copyfile, prefix + 'RF.txt', 'response.txt')
        runFunction(shutil.copyfile, prefix + 'SF.mif', 'voxels.mif')
        break
        
    lib.file.delTempFile(RF_in_path)
    lib.file.delTempFile(mask_in_path)
  # Go to the next iteration

  # If we've terminated due to hitting the iteration limiter, we still need to copy the output file(s) to the correct location
  if not os.path.exists('response.txt'):
    lib.message.console('Exiting after maximum ' + str(lib.app.args.max_iters-1) + ' iterations with ' + str(SF_voxel_count) + ' SF voxels')
    runFunction(shutil.copyfile, 'iter' + str(lib.app.args.max_iters-1) + '_RF.txt', 'response.txt')
    runFunction(shutil.copyfile, 'iter' + str(lib.app.args.max_iters-1) + '_SF.mif', 'voxels.mif')

  runFunction(shutil.copyfile, 'response.txt', lib.path.fromUser(lib.app.args.output, False))

