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

import os, shutil
from mrtrix3 import MRtrixError
from mrtrix3 import app, image, matrix, path, run



def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('tournier', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Use the Tournier et al. (2013) iterative algorithm for single-fibre voxel selection and response function estimation')
  parser.add_citation('Tournier, J.-D.; Calamante, F. & Connelly, A. Determination of the appropriate b-value and number of gradient directions for high-angular-resolution diffusion-weighted imaging. NMR Biomedicine, 2013, 26, 1775-1786')
  parser.add_argument('input', help='The input DWI')
  parser.add_argument('output', help='The output response function text file')
  options = parser.add_argument_group('Options specific to the \'tournier\' algorithm')
  options.add_argument('-number', type=int, default=300, help='Number of single-fibre voxels to use when calculating response function')
  options.add_argument('-iter_voxels', type=int, default=0, help='Number of single-fibre voxels to select when preparing for the next iteration (default = 10 x value given in -number)')
  options.add_argument('-dilate', type=int, default=1, help='Number of mask dilation steps to apply when deriving voxel mask to test in the next iteration')
  options.add_argument('-max_iters', type=int, default=10, help='Maximum number of iterations')



def check_output_paths(): #pylint: disable=unused-variable
  app.check_output_path(app.ARGS.output)



def get_inputs(): #pylint: disable=unused-variable
  pass



def needs_single_shell(): #pylint: disable=unused-variable
  return True



def supports_mask(): #pylint: disable=unused-variable
  return True



def execute(): #pylint: disable=unused-variable
  lmax_option = ''
  if app.ARGS.lmax:
    lmax_option = ' -lmax ' + app.ARGS.lmax

  if app.ARGS.max_iters < 2:
    raise MRtrixError('Number of iterations must be at least 2')

  progress = app.ProgressBar('Optimising')

  iter_voxels = app.ARGS.iter_voxels
  if iter_voxels == 0:
    iter_voxels = 10*app.ARGS.number
  elif iter_voxels < app.ARGS.number:
    raise MRtrixError ('Number of selected voxels (-iter_voxels) must be greater than number of voxels desired (-number)')

  iteration = 0
  while iteration < app.ARGS.max_iters:
    prefix = 'iter' + str(iteration) + '_'

    if iteration == 0:
      rf_in_path = 'init_RF.txt'
      mask_in_path = 'mask.mif'
      init_rf = '1 -1 1'
      with open(rf_in_path, 'w', encoding='utf-8') as init_rf_file:
        init_rf_file.write(init_rf)
      iter_lmax_option = ' -lmax 4'
    else:
      rf_in_path = 'iter' + str(iteration-1) + '_RF.txt'
      mask_in_path = 'iter' + str(iteration-1) + '_SF_dilated.mif'
      iter_lmax_option = lmax_option

    # Run CSD
    run.command('dwi2fod csd dwi.mif ' + rf_in_path + ' ' + prefix + 'FOD.mif -mask ' + mask_in_path)
    # Get amplitudes of two largest peaks, and direction of largest
    run.command('fod2fixel ' + prefix + 'FOD.mif ' + prefix + 'fixel -peak_amp peak_amps.mif -mask ' + mask_in_path + ' -fmls_no_thresholds')
    app.cleanup(prefix + 'FOD.mif')
    if iteration:
      app.cleanup(mask_in_path)
    run.command('fixel2voxel ' + os.path.join(prefix + 'fixel', 'peak_amps.mif') + ' none ' + prefix + 'amps.mif -number 2')
    run.command('mrconvert ' + prefix + 'amps.mif ' + prefix + 'first_peaks.mif -coord 3 0 -axes 0,1,2')
    run.command('mrconvert ' + prefix + 'amps.mif ' + prefix + 'second_peaks.mif -coord 3 1 -axes 0,1,2')
    app.cleanup(prefix + 'amps.mif')
    run.command('fixel2peaks ' + os.path.join(prefix + 'fixel', 'directions.mif') + ' ' + prefix + 'first_dir.mif -number 1')
    app.cleanup(prefix + 'fixel')
    # Calculate the 'cost function' Donald derived for selecting single-fibre voxels
    # https://github.com/MRtrix3/mrtrix3/pull/426
    #  sqrt(|peak1|) * (1 - |peak2| / |peak1|)^2
    run.command('mrcalc ' + prefix + 'first_peaks.mif -sqrt 1 ' + prefix + 'second_peaks.mif ' + prefix + 'first_peaks.mif -div -sub 2 -pow -mult '+ prefix + 'CF.mif')
    app.cleanup(prefix + 'first_peaks.mif')
    app.cleanup(prefix + 'second_peaks.mif')
    voxel_count = image.statistics(prefix + 'CF.mif').count
    # Select the top-ranked voxels
    run.command('mrthreshold ' + prefix + 'CF.mif -top ' + str(min([app.ARGS.number, voxel_count])) + ' ' + prefix + 'SF.mif')
    # Generate a new response function based on this selection
    run.command('amp2response dwi.mif ' + prefix + 'SF.mif ' + prefix + 'first_dir.mif ' + prefix + 'RF.txt' + iter_lmax_option)
    app.cleanup(prefix + 'first_dir.mif')

    new_rf = matrix.load_vector(prefix + 'RF.txt')
    progress.increment('Optimising (' + str(iteration+1) + ' iterations, RF: [ ' + ', '.join('{:.3f}'.format(n) for n in new_rf) + '] )')

    # Should we terminate?
    if iteration > 0:
      run.command('mrcalc ' + prefix + 'SF.mif iter' + str(iteration-1) + '_SF.mif -sub ' + prefix + 'SF_diff.mif')
      app.cleanup('iter' + str(iteration-1) + '_SF.mif')
      max_diff = image.statistics(prefix + 'SF_diff.mif').max
      app.cleanup(prefix + 'SF_diff.mif')
      if not max_diff:
        app.cleanup(prefix + 'CF.mif')
        run.function(shutil.copyfile, prefix + 'RF.txt', 'response.txt')
        run.function(shutil.move, prefix + 'SF.mif', 'voxels.mif')
        break

    # Select a greater number of top single-fibre voxels, and dilate (within bounds of initial mask);
    #   these are the voxels that will be re-tested in the next iteration
    run.command('mrthreshold ' + prefix + 'CF.mif -top ' + str(min([iter_voxels, voxel_count])) + ' - | maskfilter - dilate - -npass ' + str(app.ARGS.dilate) + ' | mrcalc mask.mif - -mult ' + prefix + 'SF_dilated.mif')
    app.cleanup(prefix + 'CF.mif')

    iteration += 1

  progress.done()

  # If terminating due to running out of iterations, still need to put the results in the appropriate location
  if os.path.exists('response.txt'):
    app.console('Convergence of SF voxel selection detected at iteration ' + str(iteration+1))
  else:
    app.console('Exiting after maximum ' + str(app.ARGS.max_iters) + ' iterations')
    run.function(shutil.copyfile, 'iter' + str(app.ARGS.max_iters-1) + '_RF.txt', 'response.txt')
    run.function(shutil.move, 'iter' + str(app.ARGS.max_iters-1) + '_SF.mif', 'voxels.mif')

  run.function(shutil.copyfile, 'response.txt', path.from_user(app.ARGS.output, False))
  if app.ARGS.voxels:
    run.command('mrconvert voxels.mif ' + path.from_user(app.ARGS.voxels), mrconvert_keyval=path.from_user(app.ARGS.input, False), force=app.FORCE_OVERWRITE)
