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

from . import LMAXES_MULTI
from . import LMAXES_SINGLE
from . import THRESHOLD_DEFAULT

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('mtnorm', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au) and Arshiya Sangchooli (asangchooli@student.unimelb.edu.au)')
  parser.set_synopsis('Derives a DWI brain mask by calculating and then thresholding a sum-of-tissue-densities image')
  parser.add_description('This script attempts to identify brain voxels based on the total density of macroscopic '
                         'tissues as estimated through multi-tissue deconvolution. Following response function '
                         'estimation and multi-tissue deconvolution, the sum of tissue densities is thresholded at a '
                         'fixed value (default is ' + str(THRESHOLD_DEFAULT) + '), and subsequent mask image cleaning '
                         'operations are performed.')
  parser.add_description('The operation of this script is a subset of that performed by the script "dwibiasnormmask". '
                         'Many users may find that comprehensive solution preferable; this dwi2mask algorithm is nevertheless '
                         'provided to demonstrate specifically the mask estimation portion of that command.')
  parser.add_description('The ODFs estimated within this optimisation procedure are by default of lower maximal spherical harmonic '
                         'degree than what would be advised for analysis. This is done for computational efficiency. This '
                         'behaviour can be modified through the -lmax command-line option.')
  parser.add_citation('Jeurissen, B; Tournier, J-D; Dhollander, T; Connelly, A & Sijbers, J. '
                      'Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data. '
                      'NeuroImage, 2014, 103, 411-426')
  parser.add_citation('Dhollander, T.; Raffelt, D. & Connelly, A. '
                      'Unsupervised 3-tissue response function estimation from single-shell or multi-shell diffusion MR data without a co-registered T1 image. '
                      'ISMRM Workshop on Breaking the Barriers of Diffusion MRI, 2016, 5')
  parser.add_argument('input', help='The input DWI series')
  parser.add_argument('output', help='The output mask image')
  options = parser.add_argument_group('Options specific to the "mtnorm" algorithm')
  options.add_argument('-init_mask',
                       metavar='image',
                       help='Provide an initial brain mask, which will constrain the response function estimation '
                            '(if omitted, the default dwi2mask algorithm will be used)')
  options.add_argument('-lmax',
                       metavar='values',
                       help='The maximum spherical harmonic degree for the estimated FODs (see Description); '
                            'defaults are "' + ','.join(str(item) for item in LMAXES_MULTI) + '" for multi-shell and "' + ','.join(str(item) for item in LMAXES_SINGLE) + '" for single-shell data)')
  options.add_argument('-threshold',
                       type=float,
                       metavar='value',
                       default=THRESHOLD_DEFAULT,
                       help='the threshold on the total tissue density sum image used to derive the brain mask; default is ' + str(THRESHOLD_DEFAULT))
  options.add_argument('-tissuesum', metavar='image', help='Export the tissue sum image that was used to generate the mask')
