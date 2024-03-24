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
  parser = subparsers.add_parser('dhollander', parents=[base_parser])
  parser.set_author('Thijs Dhollander (thijs.dhollander@gmail.com)')
  parser.set_synopsis('Unsupervised estimation of WM, GM and CSF response functions that does not require a T1 image (or segmentation thereof)')
  parser.add_description('This is an improved version of the Dhollander et al. (2016) algorithm for unsupervised estimation of WM, GM and CSF response functions, which includes the Dhollander et al. (2019) improvements for single-fibre WM response function estimation (prior to this update, the "dwi2response tournier" algorithm had been utilised specifically for the single-fibre WM response function estimation step).')
  parser.add_citation('Dhollander, T.; Raffelt, D. & Connelly, A. Unsupervised 3-tissue response function estimation from single-shell or multi-shell diffusion MR data without a co-registered T1 image. ISMRM Workshop on Breaking the Barriers of Diffusion MRI, 2016, 5')
  parser.add_citation('Dhollander, T.; Mito, R.; Raffelt, D. & Connelly, A. Improved white matter response function estimation for 3-tissue constrained spherical deconvolution. Proc Intl Soc Mag Reson Med, 2019, 555',
                      condition='If -wm_algo option is not used')
  parser.add_argument('input', help='Input DWI dataset')
  parser.add_argument('out_sfwm', help='Output single-fibre WM response function text file')
  parser.add_argument('out_gm', help='Output GM response function text file')
  parser.add_argument('out_csf', help='Output CSF response function text file')
  options = parser.add_argument_group('Options for the \'dhollander\' algorithm')
  options.add_argument('-erode', type=int, default=3, help='Number of erosion passes to apply to initial (whole brain) mask. Set to 0 to not erode the brain mask. (default: 3)')
  options.add_argument('-fa', type=float, default=0.2, help='FA threshold for crude WM versus GM-CSF separation. (default: 0.2)')
  options.add_argument('-sfwm', type=float, default=0.5, help='Final number of single-fibre WM voxels to select, as a percentage of refined WM. (default: 0.5 per cent)')
  options.add_argument('-gm', type=float, default=2.0, help='Final number of GM voxels to select, as a percentage of refined GM. (default: 2 per cent)')
  options.add_argument('-csf', type=float, default=10.0, help='Final number of CSF voxels to select, as a percentage of refined CSF. (default: 10 per cent)')
  options.add_argument('-wm_algo', metavar='algorithm', choices=WM_ALGOS, help='Use external dwi2response algorithm for WM single-fibre voxel selection (options: ' + ', '.join(WM_ALGOS) + ') (default: built-in Dhollander 2019)')
