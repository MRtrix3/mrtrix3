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

from . import DEFAULT_CLEAN_SCALE, DEFAULT_MAX_ITERS

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
