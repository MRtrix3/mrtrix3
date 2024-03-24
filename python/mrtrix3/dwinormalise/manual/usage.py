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
from . import DEFAULT_TARGET_INTENSITY

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('manual', parents=[base_parser])
  parser.set_author('Robert E. Smith (robert.smith@florey.edu.au) and David Raffelt (david.raffelt@florey.edu.au)')
  parser.set_synopsis('Intensity normalise a DWI series based on the b=0 signal within a manually-supplied supplied mask')
  parser.add_argument('input_dwi', help='The input DWI series')
  parser.add_argument('input_mask', help='The mask within which a reference b=0 intensity will be sampled')
  parser.add_argument('output_dwi', help='The output intensity-normalised DWI series')
  parser.add_argument('-intensity', type=float, default=DEFAULT_TARGET_INTENSITY, help='Normalise the b=0 signal to a specified value (Default: ' + str(DEFAULT_TARGET_INTENSITY) + ')')
  parser.add_argument('-percentile', type=int, help='Define the percentile of the b=0 image intensties within the mask used for normalisation; if this option is not supplied then the median value (50th percentile) will be normalised to the desired intensity value')
  app.add_dwgrad_import_options(parser)
