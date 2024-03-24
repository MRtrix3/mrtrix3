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

from . import OPT_N4_BIAS_FIELD_CORRECTION

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('ants', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au)')
  parser.set_synopsis('Perform DWI bias field correction using the N4 algorithm as provided in ANTs')
  parser.add_citation('Tustison, N.; Avants, B.; Cook, P.; Zheng, Y.; Egan, A.; Yushkevich, P. & Gee, J. N4ITK: Improved N3 Bias Correction. IEEE Transactions on Medical Imaging, 2010, 29, 1310-1320', is_external=True)
  ants_options = parser.add_argument_group('Options for ANTs N4BiasFieldCorrection command')
  for key in sorted(OPT_N4_BIAS_FIELD_CORRECTION):
    ants_options.add_argument('-ants.'+key, metavar=OPT_N4_BIAS_FIELD_CORRECTION[key][0], help='N4BiasFieldCorrection option -%s. %s' % (key,OPT_N4_BIAS_FIELD_CORRECTION[key][1]))
  parser.add_argument('input',  help='The input image series to be corrected')
  parser.add_argument('output', help='The output corrected image series')
