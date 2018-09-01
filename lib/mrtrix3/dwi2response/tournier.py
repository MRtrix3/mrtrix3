def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('tournier', parents=[base_parser])
  parser.setAuthor('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.setSynopsis('Use the Tournier et al. (2013) iterative algorithm for single-fibre voxel selection and response function estimation')
  parser.addCitation('', 'Tournier, J.-D.; Calamante, F. & Connelly, A. Determination of the appropriate b-value and number of gradient directions for high-angular-resolution diffusion-weighted imaging. NMR Biomedicine, 2013, 26, 1775-1786', False)
  parser.add_argument('input', help='The input DWI')
  parser.add_argument('output', help='The output response function text file')
  options = parser.add_argument_group('Options specific to the \'tournier\' algorithm')
  options.add_argument('-iter_voxels', type=int, default=3000, help='Number of single-fibre voxels to select when preparing for the next iteration')
  options.add_argument('-sf_voxels', type=int, default=300, help='Number of single-fibre voxels to use when calculating response function')
  options.add_argument('-dilate', type=int, default=1, help='Number of mask dilation steps to apply when deriving voxel mask to test in the next iteration')
  options.add_argument('-max_iters', type=int, default=10, help='Maximum number of iterations')



def checkOutputPaths(): #pylint: disable=unused-variable
  from mrtrix3 import app
  app.checkOutputPath(app.args.output)



def getInputs(): #pylint: disable=unused-variable
  pass



def needsSingleShell(): #pylint: disable=unused-variable
  return True



def execute(): #pylint: disable=unused-variable
  import os, shutil
  from mrtrix3 import app, file, image, MRtrixException, path, run #pylint: disable=redefined-builtin

  lmax_option = ''
  if app.args.lmax:
    lmax_option = ' -lmax ' + app.args.lmax

  if app.args.max_iters < 2:
    raise MRtrixException('Number of iterations must be at least 2')

  for iteration in range(0, app.args.max_iters):
    prefix = 'iter' + str(iteration) + '_'

    if iteration == 0:
      RF_in_path = 'init_RF.txt'
      mask_in_path = 'mask.mif'
      init_RF = '1 -1 1'
      with open(RF_in_path, 'w') as f:
        f.write(init_RF)
      iter_lmax_option = ' -lmax 4'
    else:
      RF_in_path = 'iter' + str(iteration-1) + '_RF.txt'
      mask_in_path = 'iter' + str(iteration-1) + '_SF_dilated.mif'
      iter_lmax_option = lmax_option

    # Run CSD
    run.command('dwi2fod csd dwi.mif ' + RF_in_path + ' ' + prefix + 'FOD.mif -mask ' + mask_in_path + iter_lmax_option)
    # Get amplitudes of two largest peaks, and direction of largest
    run.command('fod2fixel ' + prefix + 'FOD.mif ' + prefix + 'fixel -peak peaks.mif -mask ' + mask_in_path + ' -fmls_no_thresholds')
    file.delTemporary(prefix + 'FOD.mif')
    if iteration:
      file.delTemporary(mask_in_path)
    run.command('fixel2voxel ' + prefix + 'fixel/peaks.mif split_data ' + prefix + 'amps.mif -number 2')
    run.command('mrconvert ' + prefix + 'amps.mif ' + prefix + 'first_peaks.mif -coord 3 0 -axes 0,1,2')
    run.command('mrconvert ' + prefix + 'amps.mif ' + prefix + 'second_peaks.mif -coord 3 1 -axes 0,1,2')
    file.delTemporary(prefix + 'amps.mif')
    run.command('fixel2voxel ' + prefix + 'fixel/directions.mif split_dir ' + prefix + 'all_dirs.mif -number 1')
    file.delTemporary(prefix + 'fixel')
    run.command('mrconvert ' + prefix + 'all_dirs.mif ' + prefix + 'first_dir.mif -coord 3 0:2')
    file.delTemporary(prefix + 'all_dirs.mif')
    # Calculate the 'cost function' Donald derived for selecting single-fibre voxels
    # https://github.com/MRtrix3/mrtrix3/pull/426
    #  sqrt(|peak1|) * (1 - |peak2| / |peak1|)^2
    run.command('mrcalc ' + prefix + 'first_peaks.mif -sqrt 1 ' + prefix + 'second_peaks.mif ' + prefix + 'first_peaks.mif -div -sub 2 -pow -mult '+ prefix + 'CF.mif')
    file.delTemporary(prefix + 'first_peaks.mif')
    file.delTemporary(prefix + 'second_peaks.mif')
    # Select the top-ranked voxels
    run.command('mrthreshold ' + prefix + 'CF.mif -top ' + str(app.args.sf_voxels) + ' ' + prefix + 'SF.mif')
    # Generate a new response function based on this selection
    run.command('amp2response dwi.mif ' + prefix + 'SF.mif ' + prefix + 'first_dir.mif ' + prefix + 'RF.txt' + iter_lmax_option)
    file.delTemporary(prefix + 'first_dir.mif')
    # Should we terminate?
    if iteration > 0:
      run.command('mrcalc ' + prefix + 'SF.mif iter' + str(iteration-1) + '_SF.mif -sub ' + prefix + 'SF_diff.mif')
      file.delTemporary('iter' + str(iteration-1) + '_SF.mif')
      max_diff = image.statistic(prefix + 'SF_diff.mif', 'max')
      file.delTemporary(prefix + 'SF_diff.mif')
      if int(max_diff) == 0:
        app.console('Convergence of SF voxel selection detected at iteration ' + str(iteration))
        file.delTemporary(prefix + 'CF.mif')
        run.function(shutil.copyfile, prefix + 'RF.txt', 'response.txt')
        run.function(shutil.move, prefix + 'SF.mif', 'voxels.mif')
        break

    # Select a greater number of top single-fibre voxels, and dilate (within bounds of initial mask);
    #   these are the voxels that will be re-tested in the next iteration
    run.command('mrthreshold ' + prefix + 'CF.mif -top ' + str(app.args.iter_voxels) + ' - | maskfilter - dilate - -npass ' + str(app.args.dilate) + ' | mrcalc mask.mif - -mult ' + prefix + 'SF_dilated.mif')
    file.delTemporary(prefix + 'CF.mif')

  # Commence the next iteration

  # If terminating due to running out of iterations, still need to put the results in the appropriate location
  if not os.path.exists('response.txt'):
    app.console('Exiting after maximum ' + str(app.args.max_iters) + ' iterations')
    run.function(shutil.copyfile, 'iter' + str(app.args.max_iters-1) + '_RF.txt', 'response.txt')
    run.function(shutil.move, 'iter' + str(app.args.max_iters-1) + '_SF.mif', 'voxels.mif')

  run.function(shutil.copyfile, 'response.txt', path.fromUser(app.args.output, False))
  if app.args.voxels:
    run.command('mrconvert voxels.mif ' + path.fromUser(app.args.voxels, True) + app.mrconvertOutputOption(path.fromUser(app.args.input, True)))
