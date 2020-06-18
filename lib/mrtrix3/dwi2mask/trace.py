# Copyright (c) 2008-2019 the MRtrix3 contributors.
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

from mrtrix3 import app, image, path, run

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('trace', parents=[base_parser])
  parser.set_author('Warda Syeda (wtsyeda@unimelb.edu.au) and Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('A method to generate a brain mask from trace images of b-value shells')
  parser.add_argument('input',  help='The input DWI series')
  parser.add_argument('output', help='The output mask image')
  options = parser.add_argument_group('Options specific to the \'trace\' algorithm')
  options.add_argument('-avg_all', action='store_true', help='Average volumes across all shells to create a mean image')
  options.add_argument('-shells', help='Comma separated list of shells used for masking')


def execute(): #pylint: disable=unused-variable

  # Averaging shells
  if app.ARGS.avg_all:
    run.command(('dwiextract input.mif - -shells ' + app.ARGS.shells + ' | ' if app.ARGS.shells else 'echo input.mif | ') +
                'mrmath - mean - -axis 3 | '
                'mrthreshold - - | '
                'mrconvert - mask.mif -strides +1,+2,+3')

  else:
    if app.ARGS.shells:
      run.command('dwiextract input.mif input_shells.mif -shells ' + app.ARGS.shells)
      run.command('dwishellmath input_shells.mif mean mean_shells.mif')
    else:
      run.command('dwishellmath input.mif mean mean_shells.mif')
    run.command('mrconvert mean_shells.mif -coord 3 0 meanb0.mif')

    # run per-shell histogram matching
    files = []
    for index in range((image.Header('mean_shells.mif').size()).pop()):
      filename = 'shell-{:02d}.mif'.format(index)
      run.command('mrconvert mean_shells.mif -coord 3 ' + str(index) + ' - | '
                  'mrhistmatch scale - meanb0.mif ' + filename)
      files.append(filename)

    # concatenate matched shells
    run.command('mrcat -axis 3 ' + ' '.join(files) + ' cat.mif')
    run.command('mrmath cat.mif mean - -axis 3 | mrthreshold - mask.mif')
  run.command('mrconvert mask.mif ' + path.from_user(app.ARGS.output), mrconvert_keyval=path.from_user(app.ARGS.input, False), force=app.FORCE_OVERWRITE)
