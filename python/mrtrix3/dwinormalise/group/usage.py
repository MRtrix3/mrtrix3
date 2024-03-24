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
  parser = subparsers.add_parser('group', parents=[base_parser])
  parser.set_author('David Raffelt (david.raffelt@florey.edu.au)')
  parser.set_synopsis('Performs a global DWI intensity normalisation on a group of subjects using the median b=0 white matter value as the reference')
  parser.add_description('The white matter mask is estimated from a population average FA template then warped back to each subject to perform the intensity normalisation. Note that bias field correction should be performed prior to this step.')
  parser.add_description('All input DWI files must contain an embedded diffusion gradient table; for this reason, these images must all be in either .mif or .mif.gz format.')
  parser.add_argument('input_dir', help='The input directory containing all DWI images')
  parser.add_argument('mask_dir', help='Input directory containing brain masks, corresponding to one per input image (with the same file name prefix)')
  parser.add_argument('output_dir', help='The output directory containing all of the intensity normalised DWI images')
  parser.add_argument('fa_template', help='The output population specific FA template, which is threshold to estimate a white matter mask')
  parser.add_argument('wm_mask', help='The output white matter mask (in template space), used to estimate the median b=0 white matter value for normalisation')
  parser.add_argument('-fa_threshold', default='0.4', help='The threshold applied to the Fractional Anisotropy group template used to derive an approximate white matter mask (default: 0.4)')
