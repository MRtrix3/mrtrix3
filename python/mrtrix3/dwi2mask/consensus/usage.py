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

from . import DEFAULT_THRESHOLD

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('consensus', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Generate a brain mask based on the consensus of all dwi2mask algorithms')
  parser.add_argument('input',  help='The input DWI series')
  parser.add_argument('output', help='The output mask image')
  options = parser.add_argument_group('Options specific to the "consensus" algorithm')
  options.add_argument('-algorithms', nargs='+', help='Provide a list of dwi2mask algorithms that are to be utilised')
  options.add_argument('-masks', help='Export a 4D image containing the individual algorithm masks')
  options.add_argument('-template', metavar='TemplateImage MaskImage', nargs=2, help='Provide a template image and corresponding mask for those algorithms requiring such')
  options.add_argument('-threshold', type=float, default=DEFAULT_THRESHOLD, help='The fraction of algorithms that must include a voxel for that voxel to be present in the final mask (default: ' + str(DEFAULT_THRESHOLD) + ')')
