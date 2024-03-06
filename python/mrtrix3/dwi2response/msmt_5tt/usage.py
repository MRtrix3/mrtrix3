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

from . import WM_ALGOS

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('msmt_5tt', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Derive MSMT-CSD tissue response functions based on a co-registered five-tissue-type (5TT) image')
  parser.add_citation('Jeurissen, B.; Tournier, J.-D.; Dhollander, T.; Connelly, A. & Sijbers, J. Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data. NeuroImage, 2014, 103, 411-426')
  parser.add_argument('input', help='The input DWI')
  parser.add_argument('in_5tt', help='Input co-registered 5TT image')
  parser.add_argument('out_wm', help='Output WM response text file')
  parser.add_argument('out_gm', help='Output GM response text file')
  parser.add_argument('out_csf', help='Output CSF response text file')
  options = parser.add_argument_group('Options specific to the \'msmt_5tt\' algorithm')
  options.add_argument('-dirs', help='Manually provide the fibre direction in each voxel (a tensor fit will be used otherwise)')
  options.add_argument('-fa', type=float, default=0.2, help='Upper fractional anisotropy threshold for GM and CSF voxel selection (default: 0.2)')
  options.add_argument('-pvf', type=float, default=0.95, help='Partial volume fraction threshold for tissue voxel selection (default: 0.95)')
  options.add_argument('-wm_algo', metavar='algorithm', choices=WM_ALGOS, default='tournier', help='dwi2response algorithm to use for WM single-fibre voxel selection (options: ' + ', '.join(WM_ALGOS) + '; default: tournier)')
  options.add_argument('-sfwm_fa_threshold', type=float, help='Sets -wm_algo to fa and allows to specify a hard FA threshold for single-fibre WM voxels, which is passed to the -threshold option of the fa algorithm (warning: overrides -wm_algo option)')
