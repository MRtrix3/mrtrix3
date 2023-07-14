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

import math, os
from mrtrix3 import app, image, run

DEFAULT_CLEAN_SCALE = 2
DEFAULT_MAX_ITERS = 10

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('trace', parents=[base_parser])
  parser.set_author('Warda Syeda (wtsyeda@unimelb.edu.au) and Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('A method to generate a brain mask from trace images of b-value shells')
  parser.add_argument('input',  help='The input DWI series')
  parser.add_argument('output', help='The output mask image')
  options = parser.add_argument_group('Options specific to the \'trace\' algorithm')
  options.add_argument('-shells', help='Comma separated list of shells used to generate trace-weighted images for masking')
  options.add_argument('-clean_scale',
                       type=int,
                       default=DEFAULT_CLEAN_SCALE,
                       help='the maximum scale used to cut bridges. A certain maximum scale cuts '
                            'bridges up to a width (in voxels) of 2x the provided scale. Setting '
                            'this to 0 disables the mask cleaning step. (Default: ' + str(DEFAULT_CLEAN_SCALE) + ')')
  iter_options = parser.add_argument_group('Options for turning \'dwi2mask trace\' into an iterative algorithm')
  iter_options.add_argument('-iterative',
                            action='store_true',
                            help='(EXPERIMENTAL) Iteratively refine the weights for combination of per-shell trace-weighted images prior to thresholding')
  iter_options.add_argument('-max_iters', type=int, default=DEFAULT_MAX_ITERS, help='Set the maximum number of iterations for the algorithm (default: ' + str(DEFAULT_MAX_ITERS) + ')')



def get_inputs(): #pylint: disable=unused-variable
  pass



def needs_mean_bzero(): #pylint: disable=unused-variable
  return False



def execute(): #pylint: disable=unused-variable

  if app.ARGS.shells:
    run.command('dwiextract input.mif input_shells.mif -shells ' + app.ARGS.shells)
    run.command('dwishellmath input_shells.mif mean shell_traces.mif')
  else:
    run.command('dwishellmath input.mif mean shell_traces.mif')

  run.command('mrconvert shell_traces.mif -coord 3 0 -axes 0,1,2 shell-00.mif')

  # run per-shell histogram matching
  files = ['shell-00.mif']
  shell_count = image.Header('shell_traces.mif').size()[-1]
  progress = app.ProgressBar('Performing per-shell histogram matching', shell_count-1)
  for index in range(1, shell_count):
    filename = 'shell-{:02d}.mif'.format(index)
    run.command('mrconvert shell_traces.mif -coord 3 ' + str(index) + ' -axes 0,1,2 - | '
                'mrhistmatch scale - shell-00.mif ' + filename)
    files.append(filename)
    progress.increment()
  progress.done()

  # concatenate intensity-matched shells, and perform standard cleaning
  run.command(['mrmath', files, 'mean', '-', '|',
               'mrthreshold', '-', '-', '|',
               'maskfilter', '-', 'connect', '-largest', '-', '|',
               'maskfilter', '-', 'fill', '-', '|',
               'maskfilter', '-', 'clean', '-scale', str(app.ARGS.clean_scale), 'init_mask.mif'])

  if not app.ARGS.iterative:
    return 'init_mask.mif'

  # The per-shell histogram matching should be only the first pass
  # Once an initial mask has been derived, the weights with which the different
  #   shells should be revised, based on how well each shell separates brain from
  #   non-brain
  # Each shell trace image has a mean and standard deviation inside the mask, and
  #   a mean and standard deviation outside the mask
  # A shell that provides a strong separation between within-mask and outside-mask
  #   intensities should have a greater contribution toward the combined image
  # Cohen's d would be an appropriate per-shell weight

  current_mask = 'init_mask.mif'
  iteration = 0
  while True:
    current_mask_inv = os.path.splitext(current_mask)[0] + '_inv.mif'
    run.command('mrcalc 1 ' + current_mask + ' -sub ' + current_mask_inv + ' -datatype bit')
    shell_weights = []
    iteration += 1
    for index in range(0, shell_count):
      stats_inside = image.statistics('shell-{:02d}.mif'.format(index), mask=current_mask)
      stats_outside = image.statistics('shell-{:02d}.mif'.format(index), mask=current_mask_inv)
      variance = (((stats_inside.count - 1) * stats_inside.std * stats_inside.std) \
               + ((stats_outside.count - 1) * stats_outside.std * stats_outside.std)) \
               / (stats_inside.count + stats_outside.count - 2)
      cohen_d = (stats_inside.mean - stats_outside.mean) / math.sqrt(variance)
      shell_weights.append(cohen_d)
    mask_path = 'mask-{:02d}.mif'.format(iteration)
    run.command('mrcalc shell-00.mif ' + str(shell_weights[0]) + ' -mult '
                + ' -add '.join(filepath + ' ' + str(weight) + ' -mult' for filepath, weight in zip(files[1:], shell_weights[1:]))
                + ' -add - |'
                + ' mrthreshold - - |'
                + ' maskfilter - connect -largest - |'
                + ' maskfilter - fill - |'
                + ' maskfilter - clean -scale ' + str(app.ARGS.clean_scale) + ' - |'
                + ' mrcalc input_pos_mask.mif - -mult ' + mask_path + ' -datatype bit')
    mask_mismatch_path = 'mask_mismatch-{:02d}.mif'.format(iteration)
    run.command('mrcalc ' + current_mask + ' ' + mask_path + ' -sub -abs ' + mask_mismatch_path)
    if not image.statistics(mask_mismatch_path).mean:
      app.console('Terminating optimisation due to convergence of masks between iterations')
      return mask_path
    if iteration == app.ARGS.max_iters:
      app.console('Terminating optimisation due to maximum number of iterations')
      return mask_path
    current_mask = mask_path

  assert False
