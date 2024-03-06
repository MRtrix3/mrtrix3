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

from mrtrix3 import app #pylint: disable=no-name-in-module, import-outside-toplevel
from . import SUPPORTED_OPS

def usage(cmdline): #pylint: disable=unused-variable
  cmdline.set_author('Daan Christiaens (daan.christiaens@kcl.ac.uk)')
  cmdline.set_synopsis('Apply an mrmath operation to each b-value shell in a DWI series')
  cmdline.add_description('The output of this command is a 4D image, where '
                          'each volume corresponds to a b-value shell (in order of increasing b-value), and '
                          'the intensities within each volume correspond to the chosen statistic having been computed from across the DWI volumes belonging to that b-value shell.')
  cmdline.add_argument('input', help='The input diffusion MRI series')
  cmdline.add_argument('operation', choices=SUPPORTED_OPS, help='The operation to be applied to each shell; this must be one of the following: ' + ', '.join(SUPPORTED_OPS))
  cmdline.add_argument('output', help='The output image series')
  cmdline.add_example_usage('To compute the mean diffusion-weighted signal in each b-value shell',
                            'dwishellmath dwi.mif mean shellmeans.mif')
  app.add_dwgrad_import_options(cmdline)
