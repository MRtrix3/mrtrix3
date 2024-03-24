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

from mrtrix3 import app
from . import REFERENCE_INTENSITY, LMAXES_MULTI, LMAXES_SINGLE

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('mtnorm', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au) and Arshiya Sangchooli (asangchooli@student.unimelb.edu.au)')
  parser.set_synopsis('Normalise a DWI series to the estimated b=0 CSF intensity')
  parser.add_description('This algorithm determines an appropriate global scaling factor to apply to a DWI series '
                         'such that after the scaling is applied, the b=0 CSF intensity corresponds to some '
                         'reference value (' + str(REFERENCE_INTENSITY) + ' by default).')
  parser.add_description('The operation of this script is a subset of that performed by the script "dwibiasnormmask". '
                         'Many users may find that comprehensive solution preferable; this dwinormalise algorithm is '
                         'nevertheless provided to demonstrate specifically the global intensituy normalisation portion of that command.')
  parser.add_description('The ODFs estimated within this optimisation procedure are by default of lower maximal spherical harmonic '
                         'degree than what would be advised for analysis. This is done for computational efficiency. This '
                         'behaviour can be modified through the -lmax command-line option.')
  parser.add_citation('Jeurissen, B; Tournier, J-D; Dhollander, T; Connelly, A & Sijbers, J. '
                      'Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data. '
                      'NeuroImage, 2014, 103, 411-426')
  parser.add_citation('Raffelt, D.; Dhollander, T.; Tournier, J.-D.; Tabbara, R.; Smith, R. E.; Pierre, E. & Connelly, A. '
                      'Bias Field Correction and Intensity Normalisation for Quantitative Analysis of Apparent Fibre Density. '
                      'In Proc. ISMRM, 2017, 26, 3541')
  parser.add_citation('Dhollander, T.; Tabbara, R.; Rosnarho-Tornstrand, J.; Tournier, J.-D.; Raffelt, D. & Connelly, A. '
                      'Multi-tissue log-domain intensity and inhomogeneity normalisation for quantitative apparent fibre density. '
                      'In Proc. ISMRM, 2021, 29, 2472')
  parser.add_citation('Dhollander, T.; Raffelt, D. & Connelly, A. '
                      'Unsupervised 3-tissue response function estimation from single-shell or multi-shell diffusion MR data without a co-registered T1 image. '
                      'ISMRM Workshop on Breaking the Barriers of Diffusion MRI, 2016, 5')
  parser.add_argument('input', help='The input DWI series')
  parser.add_argument('output', help='The normalised DWI series')
  options = parser.add_argument_group('Options specific to the "mtnorm" algorithm')
  options.add_argument('-lmax',
                       metavar='values',
                       help='The maximum spherical harmonic degree for the estimated FODs (see Description); '
                            'defaults are "' + ','.join(str(item) for item in LMAXES_MULTI) + '" for multi-shell and "' + ','.join(str(item) for item in LMAXES_SINGLE) + '" for single-shell data)')
  options.add_argument('-mask',
                       metavar='image',
                       help='Provide a mask image for relevant calculations '
                            '(if not provided, the default dwi2mask algorithm will be used)')
  options.add_argument('-reference',
                       type=float,
                       metavar='value',
                       default=REFERENCE_INTENSITY,
                       help='Set the target CSF b=0 intensity in the output DWI series '
                            '(default: ' + str(REFERENCE_INTENSITY) + ')')
  options.add_argument('-scale',
                       metavar='file',
                       help='Write the scaling factor applied to the DWI series to a text file')
  app.add_dwgrad_import_options(parser)
