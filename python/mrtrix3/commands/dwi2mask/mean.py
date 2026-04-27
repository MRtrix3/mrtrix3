# Copyright (c) 2008-2026 the MRtrix3 contributors.
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

from mrtrix3 import app, run

NEEDS_MEAN_BZERO = False # pylint: disable=unused-variable
DEFAULT_CLEAN_SCALE = 2

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('mean', parents=[base_parser])
  parser.set_author('Warda Syeda (wtsyeda@unimelb.edu.au)')
  parser.set_synopsis('Generate a mask based on simply averaging all volumes in the DWI series')
  parser.add_argument('input',
                      type=app.Parser.ImageIn(),
                      help='The input DWI series')
  parser.add_argument('output',
                      type=app.Parser.ImageOut(),
                      help='The output mask image')
  options = parser.add_argument_group('Options specific to the "mean" algorithm')
  options.add_argument('-shells',
                       type=app.Parser.SequenceFloat(),
                       metavar='bvalues',
                       help='Comma separated list of shells to be included in the volume averaging')
  options.add_argument('-clean_scale',
                       type=app.Parser.Int(0),
                       default=DEFAULT_CLEAN_SCALE,
                       help='the maximum scale used to cut bridges. '
                            'A certain maximum scale cuts bridges up to a width (in voxels) of 2x the provided scale. '
                            'Setting this to 0 disables the mask cleaning step. '
                            f'(Default: {DEFAULT_CLEAN_SCALE})')



def execute(): #pylint: disable=unused-variable

  run.command(('dwiextract input.mif - -shells ' + ','.join(map(str, app.ARGS.shells)) + ' | mrmath -' \
                if app.ARGS.shells \
                else 'mrmath input.mif') +
              ' mean - -axis 3 |'
              ' mrthreshold - - |'
              ' maskfilter - connect -largest - |'
              ' maskfilter - fill - |'
              f' maskfilter - clean -scale {app.ARGS.clean_scale} mask.mif')

  return 'mask.mif'
