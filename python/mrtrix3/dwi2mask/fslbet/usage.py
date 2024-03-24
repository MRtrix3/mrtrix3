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

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('fslbet', parents=[base_parser])
  parser.set_author('Warda Syeda (wtsyeda@unimelb.edu.au) and Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Use the FSL Brain Extraction Tool (bet) to generate a brain mask')
  parser.add_citation('Smith, S. M. Fast robust automated brain extraction. Human Brain Mapping, 2002, 17, 143-155', is_external=True)
  parser.add_argument('input',  help='The input DWI series')
  parser.add_argument('output', help='The output mask image')
  options = parser.add_argument_group('Options specific to the \'fslbet\' algorithm')
  options.add_argument('-bet_f', type=float, help='Fractional intensity threshold (0->1); smaller values give larger brain outline estimates')
  options.add_argument('-bet_g', type=float, help='Vertical gradient in fractional intensity threshold (-1->1); positive values give larger brain outline at bottom, smaller at top')
  options.add_argument('-bet_c', nargs=3, metavar='<x y z>', help='Centre-of-gravity (voxels not mm) of initial mesh surface')
  options.add_argument('-bet_r', type=float, help='Head radius (mm not voxels); initial surface sphere is set to half of this')
  options.add_argument('-rescale', action='store_true', help='Rescale voxel size provided to BET to 1mm isotropic; can improve results for rodent data')
