def initialise(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('tax', author='Robert E. Smith (robert.smith@florey.edu.au)', synopsis='Use the Tax et al. (2014) recursive calibration algorithm for single-fibre voxel selection and response function estimation', parents=[base_parser])
  parser.addCitation('', 'Tax, C. M.; Jeurissen, B.; Vos, S. B.; Viergever, M. A. & Leemans, A. Recursive calibration of the fiber response function for spherical deconvolution of diffusion MRI data. NeuroImage, 2014, 86, 67-80', False)
  parser.add_argument('input', help='The input DWI')
  parser.add_argument('output', help='The output response function text file')
  options = parser.add_argument_group('Options specific to the \'tax\' algorithm')
  options.add_argument('-peak_ratio', type=float, default=0.1, help='Second-to-first-peak amplitude ratio threshold')
  options.add_argument('-max_iters', type=int, default=20, help='Maximum number of iterations')
  options.add_argument('-convergence', type=float, default=0.5, help='Percentile change in any RF coefficient required to continue iterating')



def checkOutputPaths(): #pylint: disable=unused-variable
  from mrtrix3 import app
  app.checkOutputPath(app.args.output)



def getInputs(): #pylint: disable=unused-variable
  pass



def needsSingleShell(): #pylint: disable=unused-variable
  return True



def execute(): #pylint: disable=unused-variable
  import math, os, shutil
  from mrtrix3 import app, file, image, path, run

  lmax_option = ''
  if app.args.lmax:
    lmax_option = ' -lmax ' + app.args.lmax

  convergence_change = 0.01 * app.args.convergence

  for iteration in range(0, app.args.max_iters):
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

      # Grab the mean and standard deviation across all volumes in a single mrstats call
      # Also scale them to reflect the fact that we're moving to the SH basis
      mean = float(image.statistic('dwi.mif', 'mean', '-mask mask.mif -allvolumes')) * math.sqrt(4.0 * math.pi)
      std = float(image.statistic('dwi.mif', 'std', '-mask mask.mif -allvolumes')) * math.sqrt(4.0 * math.pi)

      # Now produce the initial response function
      # Let's only do it to lmax 4
      init_RF = [ str(mean), str(-0.5*std), str(0.25*std*std/mean) ]
      with open('init_RF.txt', 'w') as f:
        f.write(' '.join(init_RF))
    else:
      RF_in_path = 'iter' + str(iteration-1) + '_RF.txt'
      mask_in_path = 'iter' + str(iteration-1) + '_SF.mif'

    # Run CSD
    run.command('dwi2fod csd dwi.mif ' + RF_in_path + ' ' + prefix + 'FOD.mif -mask ' + mask_in_path)
    # Get amplitudes of two largest peaks, and directions of largest
    run.command('fod2fixel ' + prefix + 'FOD.mif ' + prefix + 'fixel -peak peaks.mif -mask ' + mask_in_path + ' -fmls_no_thresholds')
    file.delTemporary(prefix + 'FOD.mif')
    run.command('fixel2voxel ' + prefix + 'fixel/peaks.mif split_data ' + prefix + 'amps.mif')
    run.command('mrconvert ' + prefix + 'amps.mif ' + prefix + 'first_peaks.mif -coord 3 0 -axes 0,1,2')
    run.command('mrconvert ' + prefix + 'amps.mif ' + prefix + 'second_peaks.mif -coord 3 1 -axes 0,1,2')
    file.delTemporary(prefix + 'amps.mif')
    run.command('fixel2voxel ' + prefix + 'fixel/directions.mif split_dir ' + prefix + 'all_dirs.mif')
    file.delTemporary(prefix + 'fixel')
    run.command('mrconvert ' + prefix + 'all_dirs.mif ' + prefix + 'first_dir.mif -coord 3 0:2')
    file.delTemporary(prefix + 'all_dirs.mif')
    # Revise single-fibre voxel selection based on ratio of tallest to second-tallest peak
    run.command('mrcalc ' + prefix + 'second_peaks.mif ' + prefix + 'first_peaks.mif -div ' + prefix + 'peak_ratio.mif')
    file.delTemporary(prefix + 'first_peaks.mif')
    file.delTemporary(prefix + 'second_peaks.mif')
    run.command('mrcalc ' + prefix + 'peak_ratio.mif ' + str(app.args.peak_ratio) + ' -lt ' + mask_in_path + ' -mult ' + prefix + 'SF.mif -datatype bit')
    file.delTemporary(prefix + 'peak_ratio.mif')
    # Make sure image isn't empty
    SF_voxel_count = int(image.statistic(prefix + 'SF.mif', 'count', '-mask ' + prefix + 'SF.mif'))
    if not SF_voxel_count:
      app.error('Aborting: All voxels have been excluded from single-fibre selection')
    # Generate a new response function
    run.command('amp2response dwi.mif ' + prefix + 'SF.mif ' + prefix + 'first_dir.mif ' + prefix + 'RF.txt' + lmax_option)
    file.delTemporary(prefix + 'first_dir.mif')

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
        app.console('Exiting at iteration ' + str(iteration) + ' with ' + str(SF_voxel_count) + ' SF voxels due to unchanged response function coefficients')
        run.function(shutil.copyfile, prefix + 'RF.txt', 'response.txt')
        run.function(shutil.copyfile, prefix + 'SF.mif', 'voxels.mif')
        break

    file.delTemporary(RF_in_path)
    file.delTemporary(mask_in_path)
  # Go to the next iteration

  # If we've terminated due to hitting the iteration limiter, we still need to copy the output file(s) to the correct location
  if not os.path.exists('response.txt'):
    app.console('Exiting after maximum ' + str(app.args.max_iters-1) + ' iterations with ' + str(SF_voxel_count) + ' SF voxels')
    run.function(shutil.copyfile, 'iter' + str(app.args.max_iters-1) + '_RF.txt', 'response.txt')
    run.function(shutil.copyfile, 'iter' + str(app.args.max_iters-1) + '_SF.mif', 'voxels.mif')

  run.function(shutil.copyfile, 'response.txt', path.fromUser(app.args.output, False))
