# Copyright (c) 2008-2020 the MRtrix3 contributors.
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

from mrtrix3 import app, image, run

DEFAULT_CLEAN_SCALE = 2

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('trace', parents=[base_parser])
  parser.set_author('Warda Syeda (wtsyeda@unimelb.edu.au) and Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('A method to generate a brain mask from trace images of b-value shells')
  parser.add_argument('input',  help='The input DWI series')
  parser.add_argument('output', help='The output mask image')
  options = parser.add_argument_group('Options specific to the \'trace\' algorithm')
  options.add_argument('-avg_all', action='store_true', help='Average DWI volumes directly to create an image for thresholding')
  options.add_argument('-shells', help='Comma separated list of shells used for masking')
  parser.add_argument('-clean_scale',
                      type=int,
                      default=DEFAULT_CLEAN_SCALE,
                      help='the maximum scale used to cut bridges. A certain maximum scale cuts '
                           'bridges up to a width (in voxels) of 2x the provided scale. Setting '
                           'this to 0 disables the mask cleaning step. (Default: ' + str(DEFAULT_CLEAN_SCALE) + ')')



def get_inputs(): #pylint: disable=unused-variable
  pass



def needs_mean_bzero(): #pylint: disable=unused-variable
  return False



def execute(): #pylint: disable=unused-variable

  # Averaging volumes
  if app.ARGS.avg_all:
    run.command(('dwiextract input.mif - -shells ' + app.ARGS.shells + ' | mrmath -' \
                 if app.ARGS.shells \
                 else 'mrmath input.mif')
                + ' mean - -axis 3 |'
                + ' mrthreshold - init_mask.mif')

  else:

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
      run.command('mrconvert shell_traces.mif -coord 3 ' + str(index) + ' - | '
                  'mrhistmatch scale - shell-00.mif ' + filename)
      files.append(filename)
      progress.increment()
    progress.done()

    # concatenate matched shells
    run.command(['mrmath', files, 'mean', '-', '|',
                 'mrthreshold', '-', 'init_mask.mif'])

  # Cleaning the mask
  run.command('maskfilter init_mask.mif connect -largest - | '
              'mrcalc 1 - -sub - | '
              'maskfilter - connect -largest - | '
              'mrcalc 1 - -sub - | '
              'maskfilter - clean -scale ' + str(app.ARGS.clean_scale) + ' cleaned_mask.mif')
  run.command('mrmath input.mif max -axis 3 - | '
              'mrcalc - 0.0 -gt cleaned_mask.mif -mult final_mask.mif -datatype bit')

  return 'final_mask.mif'
