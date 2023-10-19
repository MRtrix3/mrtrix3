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

from mrtrix3 import app, run

DEFAULT_CLEAN_SCALE = 2



def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('legacy', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Use the legacy MRtrix3 dwi2mask heuristic (based on thresholded trace images)')
  parser.add_argument('input',  help='The input DWI series')
  parser.add_argument('output', help='The output mask image')
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

  run.command('mrcalc input.mif 0 -max input_nonneg.mif')
  run.command('dwishellmath input_nonneg.mif mean trace.mif')
  app.cleanup('input_nonneg.mif')
  run.command('mrthreshold trace.mif - -comparison gt | '
              'mrmath - max -axis 3 - | '
              'maskfilter - median - | '
              'maskfilter - connect -largest - | '
              'maskfilter - fill - | '
              'maskfilter - clean -scale ' + str(app.ARGS.clean_scale) + ' mask.mif')

  return 'mask.mif'
