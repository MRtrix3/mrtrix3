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


def usage(base_parser, subcommands): #pylint: disable=unused-variable
  parser = subcommands.add_subcommand('legacy', parent=base_parser)
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Use the legacy MRtrix3 dwi2mask heuristic (based on thresholded trace images)')
  parser.add_argument('input',
                      'The input DWI series',
                      type=app.Parser.ImageIn())
  parser.add_argument('output',
                      'The output mask image',
                      type=app.Parser.ImageOut())
  parser.add_option('clean_scale',
                    'the maximum scale used to cut bridges. '
                    'A certain maximum scale cuts bridges up to a width (in voxels) of 2x the provided scale. '
                    'Setting this to 0 disables the mask cleaning step.',
                    type=app.Parser.Int(0),
                    default=DEFAULT_CLEAN_SCALE)



def execute(): #pylint: disable=unused-variable

  run.command('mrcalc input.mif 0 -max - | '
              'dwishellmath - mean - | '
              'mrthreshold - - -comparison gt | '
              'mrmath - max -axis 3 - | '
              'maskfilter - median - | '
              'maskfilter - connect -largest - | '
              'maskfilter - fill - | '
              f'maskfilter - clean -scale {app.ARGS.clean_scale} mask.mif')

  return 'mask.mif'
