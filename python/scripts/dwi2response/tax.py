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

import math, os, shutil
from mrtrix3 import MRtrixError
from mrtrix3 import app, image, matrix, path, run



def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('tax', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Use the Tax et al. (2014) recursive calibration algorithm for single-fibre voxel selection and response function estimation')
  parser.add_citation('Tax, C. M.; Jeurissen, B.; Vos, S. B.; Viergever, M. A. & Leemans, A. Recursive calibration of the fiber response function for spherical deconvolution of diffusion MRI data. NeuroImage, 2014, 86, 67-80')
  parser.add_argument('input', help='The input DWI')
  parser.add_argument('output', help='The output response function text file')
  options = parser.add_argument_group('Options specific to the \'tax\' algorithm')
  options.add_argument('-peak_ratio', type=float, default=0.1, help='Second-to-first-peak amplitude ratio threshold')
  options.add_argument('-max_iters', type=int, default=20, help='Maximum number of iterations')
  options.add_argument('-convergence', type=float, default=0.5, help='Percentile change in any RF coefficient required to continue iterating')



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

  convergence_change = 0.01 * app.ARGS.convergence

  progress = app.ProgressBar('Optimising')

  iteration = 0
  while iteration < app.ARGS.max_iters:
    prefix = 'iter' + str(iteration) + '_'

    # How to initialise response function?
    # old dwi2response command used mean & standard deviation of DWI data; however
    #   this may force the output FODs to lmax=2 at the first iteration
    # Chantal used a tensor with low FA, but it'd be preferable to get the scaling right
    # Other option is to do as before, but get the ratio between l=0 and l=2, and
    #   generate l=4,6,... using that amplitude ratio
    if iteration == 0:
      rf_in_path = 'init_RF.txt'
      mask_in_path = 'mask.mif'

      # Grab the mean and standard deviation across all volumes in a single mrstats call
      # Also scale them to reflect the fact that we're moving to the SH basis
      image_stats = image.statistics('dwi.mif', mask='mask.mif', allvolumes=True)
      mean = image_stats.mean * math.sqrt(4.0 * math.pi)
      std = image_stats.std * math.sqrt(4.0 * math.pi)

      # Now produce the initial response function
      # Let's only do it to lmax 4
      init_rf = [ str(mean), str(-0.5*std), str(0.25*std*std/mean) ]
      with open('init_RF.txt', 'w', encoding='utf-8') as init_rf_file:
        init_rf_file.write(' '.join(init_rf))
    else:
      rf_in_path = 'iter' + str(iteration-1) + '_RF.txt'
      mask_in_path = 'iter' + str(iteration-1) + '_SF.mif'

    # Run CSD
    run.command('dwi2fod csd dwi.mif ' + rf_in_path + ' ' + prefix + 'FOD.mif -mask ' + mask_in_path)
    # Get amplitudes of two largest peaks, and directions of largest
    run.command('fod2fixel ' + prefix + 'FOD.mif ' + prefix + 'fixel -peak peaks.mif -mask ' + mask_in_path + ' -fmls_no_thresholds')
    app.cleanup(prefix + 'FOD.mif')
    run.command('fixel2voxel ' + prefix + 'fixel/peaks.mif none ' + prefix + 'amps.mif')
    run.command('mrconvert ' + prefix + 'amps.mif ' + prefix + 'first_peaks.mif -coord 3 0 -axes 0,1,2')
    run.command('mrconvert ' + prefix + 'amps.mif ' + prefix + 'second_peaks.mif -coord 3 1 -axes 0,1,2')
    app.cleanup(prefix + 'amps.mif')
    run.command('fixel2peaks ' + prefix + 'fixel/directions.mif ' + prefix + 'first_dir.mif -number 1')
    app.cleanup(prefix + 'fixel')
    # Revise single-fibre voxel selection based on ratio of tallest to second-tallest peak
    run.command('mrcalc ' + prefix + 'second_peaks.mif ' + prefix + 'first_peaks.mif -div ' + prefix + 'peak_ratio.mif')
    app.cleanup(prefix + 'first_peaks.mif')
    app.cleanup(prefix + 'second_peaks.mif')
    run.command('mrcalc ' + prefix + 'peak_ratio.mif ' + str(app.ARGS.peak_ratio) + ' -lt ' + mask_in_path + ' -mult ' + prefix + 'SF.mif -datatype bit')
    app.cleanup(prefix + 'peak_ratio.mif')
    # Make sure image isn't empty
    sf_voxel_count = image.statistics(prefix + 'SF.mif', mask=prefix+'SF.mif').count
    if not sf_voxel_count:
      raise MRtrixError('Aborting: All voxels have been excluded from single-fibre selection')
    # Generate a new response function
    run.command('amp2response dwi.mif ' + prefix + 'SF.mif ' + prefix + 'first_dir.mif ' + prefix + 'RF.txt' + lmax_option)
    app.cleanup(prefix + 'first_dir.mif')

    new_rf = matrix.load_vector(prefix + 'RF.txt')
    progress.increment('Optimising (' + str(iteration+1) + ' iterations, ' + str(sf_voxel_count) + ' voxels, RF: [ ' + ', '.join('{:.3f}'.format(n) for n in new_rf) + '] )')

    # Detect convergence
    # Look for a change > some percentage - don't bother looking at the masks
    if iteration > 0:
      old_rf = matrix.load_vector(rf_in_path)
      reiterate = False
      for old_value, new_value in zip(old_rf, new_rf):
        mean = 0.5 * (old_value + new_value)
        diff = math.fabs(0.5 * (old_value - new_value))
        ratio = diff / mean
        if ratio > convergence_change:
          reiterate = True
      if not reiterate:
        run.function(shutil.copyfile, prefix + 'RF.txt', 'response.txt')
        run.function(shutil.copyfile, prefix + 'SF.mif', 'voxels.mif')
        break

    app.cleanup(rf_in_path)
    app.cleanup(mask_in_path)

    iteration += 1

  progress.done()

  # If we've terminated due to hitting the iteration limiter, we still need to copy the output file(s) to the correct location
  if os.path.exists('response.txt'):
    app.console('Exited at iteration ' + str(iteration+1) + ' with ' + str(sf_voxel_count) + ' SF voxels due to unchanged RF coefficients')
  else:
    app.console('Exited after maximum ' + str(app.ARGS.max_iters) + ' iterations with ' + str(sf_voxel_count) + ' SF voxels')
    run.function(shutil.copyfile, 'iter' + str(app.ARGS.max_iters-1) + '_RF.txt', 'response.txt')
    run.function(shutil.copyfile, 'iter' + str(app.ARGS.max_iters-1) + '_SF.mif', 'voxels.mif')

  run.function(shutil.copyfile, 'response.txt', path.from_user(app.ARGS.output, False))
  if app.ARGS.voxels:
    run.command('mrconvert voxels.mif ' + path.from_user(app.ARGS.voxels), mrconvert_keyval=path.from_user(app.ARGS.input, False), force=app.FORCE_OVERWRITE)
