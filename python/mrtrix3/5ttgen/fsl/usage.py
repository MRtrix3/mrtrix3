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
  parser = subparsers.add_parser('fsl', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Use FSL commands to generate the 5TT image based on a T1-weighted image')
  parser.add_citation('Smith, S. M. Fast robust automated brain extraction. Human Brain Mapping, 2002, 17, 143-155', is_external=True)
  parser.add_citation('Zhang, Y.; Brady, M. & Smith, S. Segmentation of brain MR images through a hidden Markov random field model and the expectation-maximization algorithm. IEEE Transactions on Medical Imaging, 2001, 20, 45-57', is_external=True)
  parser.add_citation('Patenaude, B.; Smith, S. M.; Kennedy, D. N. & Jenkinson, M. A Bayesian model of shape and appearance for subcortical brain segmentation. NeuroImage, 2011, 56, 907-922', is_external=True)
  parser.add_citation('Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.; Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.; Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.; Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. Advances in functional and structural MR image analysis and implementation as FSL. NeuroImage, 2004, 23, S208-S219', is_external=True)
  parser.add_argument('input',  help='The input T1-weighted image')
  parser.add_argument('output', help='The output 5TT image')
  options = parser.add_argument_group('Options specific to the \'fsl\' algorithm')
  options.add_argument('-t2', metavar='<T2 image>', help='Provide a T2-weighted image in addition to the default T1-weighted image; this will be used as a second input to FSL FAST')
  options.add_argument('-mask', help='Manually provide a brain mask, rather than deriving one in the script')
  options.add_argument('-premasked', action='store_true', help='Indicate that brain masking has already been applied to the input image')
  parser.flag_mutually_exclusive_options( [ 'mask', 'premasked' ] )
